# libcoders

Simple library that lets you compress files (6 algorithms available: Shennon, Fano, Huffman, Bigram Huffman, Adaptive Huffman and Arithmetic coding).

Made for educational purposes.

# How-to-use:

## 1. Build, run and get help

  * Build
    ```
    $ make default
    ```
  
  * Help
    ```
    $ ./libcoders -h
    ```
  
  * Usage example
    ```
    $ ./libcoders -c -i input_file.txt -o encoded_file -m shennon
    $ ./libcoders -d -i encoded_file -o decoded_file.txt -m shennon
    ```
  
  ## 2. Clean project

  * Clean
  
    ```
    $ make clean
    ```

# GUI

There is also a GUI version of this project available [here](https://github.com/snovvcrash/LibCoders-GUI "LibCoders-GUI").
