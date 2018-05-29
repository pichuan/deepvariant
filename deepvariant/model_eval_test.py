# Copyright 2017 Google Inc.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived from this
#    software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
"""Tests for genomics.deepvariant.model_eval."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import os


from tensorflow import flags
from absl.testing import absltest
from absl.testing import parameterized
import mock
import six
import tensorflow as tf

from deepvariant import data_providers_test
from deepvariant import model_eval
from deepvariant import testdata
from deepvariant.testing import flagsaver
from deepvariant.testing import tf_test_utils

FLAGS = flags.FLAGS

# Note that this test suite is invoked twice, with --use_tpu set both ways.


def setUpModule():
  testdata.init()


class ModelEvalTest(
    six.with_metaclass(parameterized.TestGeneratorMetaclass, tf.test.TestCase)):

  def setUp(self):
    self.checkpoint_dir = tf.test.get_temp_dir()
    # Use this to generate a random name.  The framework
    # will create the directory under self.checkpoint_dir.
    self.eval_name = os.path.basename(tf.test.get_temp_dir())

  @parameterized.parameters(['inception_v3'])
  @flagsaver.FlagSaver
  @mock.patch('deepvariant.data_providers.'
              'get_input_fn_from_dataset')
  def test_end2end(self, model_name, mock_get_input_fn_from_dataset):
    """End-to-end test of model_eval."""
    tf_test_utils.write_fake_checkpoint('inception_v3', self.test_session(),
                                        self.checkpoint_dir,
                                        FLAGS.moving_average_decay)

    # Start up eval, loading that checkpoint.
    FLAGS.batch_size = 2
    FLAGS.checkpoint_dir = self.checkpoint_dir
    FLAGS.eval_name = self.eval_name
    FLAGS.max_evaluations = 1
    FLAGS.max_examples = 2
    FLAGS.model_name = model_name
    FLAGS.dataset_config_pbtxt = '/path/to/mock.pbtxt'
    FLAGS.master = ''
    # Always try to read in compressed inputs to stress that case. Uncompressed
    # inputs are certain to work. This test is expensive to run, so we want to
    # minimize the number of times we need to run this.
    mock_get_input_fn_from_dataset.return_value = (
        data_providers_test.make_golden_dataset(
            compressed_inputs=True, use_tpu=FLAGS.use_tpu))
    model_eval.main(0)
    mock_get_input_fn_from_dataset.assert_called_once_with(
        dataset_config_filename=FLAGS.dataset_config_pbtxt,
        mode=tf.estimator.ModeKeys.EVAL)

  # Using a constant model, check that running an eval returns the expected
  # metrics.
  @flagsaver.FlagSaver
  @mock.patch(
      'deepvariant.model_eval.checkpoints_iterator')
  @mock.patch('deepvariant.data_providers.'
              'get_input_fn_from_dataset')
  def test_fixed_eval_sees_the_same_evals(self, mock_get_input_fn_from_dataset,
                                          mock_checkpoints_iterator):
    dataset = data_providers_test.make_golden_dataset(use_tpu=FLAGS.use_tpu)
    n_checkpoints = 3
    checkpoints = [
        tf_test_utils.write_fake_checkpoint(
            'constant',
            self.test_session(),
            self.checkpoint_dir,
            FLAGS.moving_average_decay,
            name='model' + str(i)) for i in range(n_checkpoints)
    ]

    # Setup our mocks.
    mock_checkpoints_iterator.return_value = checkpoints
    mock_get_input_fn_from_dataset.return_value = dataset

    # Start up eval, loading that checkpoint.
    FLAGS.batch_size = 2
    FLAGS.checkpoint_dir = self.checkpoint_dir
    FLAGS.eval_name = self.eval_name
    FLAGS.max_evaluations = n_checkpoints
    FLAGS.model_name = 'constant'
    FLAGS.dataset_config_pbtxt = '/path/to/mock.pbtxt'
    FLAGS.master = ''
    model_eval.main(0)

    self.assertEqual(mock_get_input_fn_from_dataset.call_args_list, [
        mock.call(
            dataset_config_filename=FLAGS.dataset_config_pbtxt,
            mode=tf.estimator.ModeKeys.EVAL)
    ])

    metrics = [
        model_eval.read_metrics(checkpoint, eval_name=FLAGS.eval_name)
        for checkpoint in checkpoints
    ]

    # Check that our metrics are what we expect them to be.
    # See b/62864044 for details on how to compute these counts:
    # Counts of labels in our golden dataset:
    #  1 0
    # 12 1
    # 35 2
    expected_values_for_all_exact = {
        # We have 12 correct calls [there are 12 variants with a label of 1] and
        # 1 label 0 + 35 with a label of 2, so we have an accuracy of 12 / 48,
        # which is 0.25.
        'Accuracy/All': 0.25,
        # We don't have any FNs because we call everything het.
        'FNs/All': 0,
        # One of our labels is 0, which we call het, giving us 1 FP.
        'FPs/All': 1.0,
        # We call everything as het, so the recall has to be 1.
        'Recall/All': 1.0,
        # redacted
        # # We don't call anything but hets, so TNs has to be 0.
        # 'TNs/All': 0,
        # We find all positives, so this has to be 47.
        'TPs/All': 47,
    }
    for key, expected_value in expected_values_for_all_exact.iteritems():
      self.assertEqual(metrics[0][key], expected_value)

    expected_values_for_all_close = {
        # We called 47 / 48 correctly.
        'Precision/All': 47. / 48,
    }
    for key, expected_value in expected_values_for_all_close.iteritems():
      self.assertAlmostEqual(metrics[0][key], expected_value, places=6)

    for m1, m2 in zip(metrics, metrics[1:]):
      self.assertEqual(m1, m2)


if __name__ == '__main__':
  absltest.main()
