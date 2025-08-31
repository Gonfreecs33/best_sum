import pandas as pd
import matplotlib.pyplot as plt
import os

data_dir = "csv_data"
files = sorted(os.listdir(data_dir))  # lister tous les CSV

for file in files:
    if file.endswith(".csv"):
        path = os.path.join(data_dir, file)
        df = pd.read_csv(path, names=["x", "y"])
        
        plt.figure(figsize=(10, 5))
        plt.plot(df["x"], df["y"], marker='o', markersize=2)
        plt.title(f"Function: {file}")
        plt.xlabel("x")
        plt.ylabel("y")
        plt.grid(True)
        plt.tight_layout()
        plt.show()  # affiche chaque fonction une par une
