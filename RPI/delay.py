import pandas as pd
import numpy as np

samples = pd.read_csv('RPI/delay.csv')

print("Parameter aquisition \t\t\t Min:", samples['Parameter'].min(), "\t\tMax:", samples['Parameter'].max(), "\tMean:", samples['Parameter'].mean())
print("Wave generation \t\t\t Min:", samples['Wave'].min(), "\t\tMax:", samples['Wave'].max(), "\tMean:", samples['Wave'].mean())
print("Sending buffer to audio \t\t Min:", samples['Buffer'].min(), "\tMax:", samples['Buffer'].max(), "\tMean:", samples['Buffer'].mean())
print("Sending buffer to visualisation \t Min:", samples['Visualisation'].min(), "\t\tMax:", samples['Visualisation'].max(), "\tMean:", samples['Visualisation'].mean())