################################################################################

1. How to Build
	- get Toolchain
		From Codesourcery site(	http://www.codesourcery.com )
		recommand : up to 2009q3 version.
	- edit build_kernel.sh
		edit "TOOLCHAIN" and "TOOLCHAIN_PREFIX"
	- run build_kernel.sh
		$ ./build_kernel.sh

2. Output files
	- Kernel : linux-2.6.29/arch/arm/boot/zImage
	- module : modules/*/*.ko

3. How to Clean
	- run build_kernel.sh Clean
		$ ./build_kernel.sh Clean

################################################################################
