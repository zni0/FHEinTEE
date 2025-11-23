# FHEinTEE (PHEinTEE rn)
A toy project that uses Intel SGX as a PHE encryption service to generate keys and encrypt data (using Paillier cryptosystem).
# Set up build env
Opensgx was developed with Ubuntu 14-15. The easiest way to get it running is in a VM.
```
wget https://cloud-images.ubuntu.com/releases/trusty/release/ubuntu-14.04-server-cloudimg-amd64-disk1.img
mv ubuntu-14.04-server-cloudimg-amd64-disk1.img ub.qcow2
virt-customize -a  ub.qcow2 --root-password password:root
qemu-system-x86_64 \
	-enable-kvm \
	-m 10G -smp 2 \
	-hda ub.qcow2 \
	-net nic -net user,hostfwd=tcp::2222-:22 \
	-nographic
```
# Build the opensgx enclave
cd into the opensgx directory and follow the instructions to compile the SGX library. Some of the test programs may fail to compile, but it's fine as long as the user library compiles. Compile and run the sgx-server:
```
./opensgx -k
./opensgx -c user/demo/sgx-server.c
./opensgx -s user/demo/sgx-server.sgx --key sign.key
./opensgx user/demo/sgx-server.sgx user/demo/sgx-server.conf
```
# Build the client and normal (non SGX) server
cd into the normal directory and run make. This will give the `client` and `server` bins. You can run both of them without any arguments. The `test.sh` script runs client multiple times from random inputs.
