# libcoders
Simple library that lets you compress files (6 algorithms available: Shennon, Fano, Huffman, Bigram Huffman, Adaptive Huffman and Arithmetic coding).

Made for educational purposes.

## How-to-use:

### 1. Build, run and get help

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
  
  ### 2. Clean project

  * Clean
  
    ```
    $ make clean
    ```

## Post Scriptum
There is also a GUI version of this project availble right [here](https://github.com/snovvcrash/LibCoders-GUI "LibCoders-GUI").

If this tool has been useful for you, feel free to buy me a coffee :coffee:

[![Coffee](https://user-images.githubusercontent.com/23141800/44239464-1736e100-a1c2-11e8-889c-5018c692a01e.png)](https://buymeacoff.ee/snovvcrash)
