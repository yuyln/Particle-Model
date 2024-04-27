import pandas as pd
import matplotlib.pyplot as plt

data = pd.read_csv("./test.dat", header=None)
plt.plot(data[0], data[1], color="black")
plt.plot(data[0], data[3], color="red")
plt.show()

plt.plot(data[0], data[2], color="black")
plt.plot(data[0], data[4], color="red")
plt.show()
