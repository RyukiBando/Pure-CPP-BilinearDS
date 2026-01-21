# Pure-CPP-BilinearDS
Fun project that read a BMP, apply a gaussian blur and Downscale using bilinear method
## Use
Change the name of filename __line 37__ to the name of you file (has to be BMP)\
Compile the code on the same folder 
```
gcc bilinear.cpp -o bilinear
```
and execute the bilinear file
```
./bilinear
```
### Example
|          Ground Truth           |
|:-------------------------------:|
|<img src="./example/baby_GT.bmp">|

|           Bilinear              |                    Bicubic                  |
|:-------------------------------:|:-------------------------------------------:|
|<img src="./example/bilinear.bmp">|<img src="./example/bicubic.bmp">|
