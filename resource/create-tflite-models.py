import tensorflow as tf
import os
from crepe.core import build_and_load_model

# Clone and install the crepe repository https://github.com/marl/crepe.git
# Copy this file into the root directory of the crepe repository and run it
model_capacities = ['tiny', 'small', 'medium', 'large', 'full']
for model_capacity in model_capacities:
    model = build_and_load_model(model_capacity)
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    tflite_model = converter.convert()
    script_dir = os.path.dirname(os.path.realpath(__file__))
    with open(os.path.join(script_dir, 'crepe-' + model_capacity + '.tflite'), 'wb') as f:
        f.write(tflite_model)
