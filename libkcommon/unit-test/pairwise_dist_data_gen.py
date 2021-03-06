#!/usr/bin/env python

import numpy as np
from scipy.spatial.distance import pdist, squareform

np.random.seed(1234)
data = np.random.randint(0, 10, (16,4))
data = data.astype(np.float64)
data.tofile("data_dm.bin")

# upper triangular
pw_dm = np.triu(squareform(pdist(data, "cityblock")))
pw_dm = pw_dm.astype(np.float64)
pw_dm.tofile("pw_dm.bin")