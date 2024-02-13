# Dawn--compresser
一种新的压缩算法，平衡了速度和压缩率。没有使用huffman算法等进行后续优化，适用于自制操作系统或是学习压缩算法等对于解压程序速度以及解压程序代码复杂度要求较高的场景


A new compress algorithm which balance the speed and compress quality. Without using Huffman and other algorithms for subsequent processing, it is suitable for scenarios like OS_making or compressor_learning that require high decompress speed and code complexity. 


算法简介：
使用hash表优化查找，同时使用mtf对于distance、len两个数值进行优化，并使用bit级别的文件输出以进一步优化。同时，设置滑动窗口来平衡速度与压缩质量。
解压缩时间复杂度约为：O(n)，
压缩时间复杂度：O(n*m)
n为文件大小，m为滑动窗口大小。

编译：
使用MinGW-W64-builds-4.3.5编译。
