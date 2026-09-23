The main function will: 

1) Save the Data
	- Saving and reading the data can happen on one cpu using the highspeed option for the sd card. 
2) Classify the data

# Preprocessing data

1) The first step needs to take the raw buffer and perform a transform, it does this for each window slice. 
	- For streaming data, there is overlap that must be saved for the next transform
2) The second step will quantize the results of the transform using [esp-dl's functions for quantizing](https://docs.espressif.com/projects/esp-dl/en/latest/tutorials/how_to_run_model.html#quantize-input)
3) Lastly these now 16 bit or 8 bit results will be pushed into the models input tensor

# Running the Model
