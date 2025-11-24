# FHEinTEE (PHEinTEE rn)
A toy project that uses Intel SGX as a PHE encryption service to generate keys and encrypt data (using Paillier cryptosystem).
# Problem
Fully Homomorphic Encryption (FHE) allows computations to be performed directly on encrypted data. Although it has the potential to improve privacy and security of cloud computation, FHE is rarely deployed in practice due to its complexity. Users must manage large cryptographic keys, choose parameters carefully, and perform costly encryption/decryption steps. This usability barrier prevents many potential applications of FHE in cloud computing, data analytics, and machine learning.

Another approach to solve the confidentiality problem is to use hardwarebased isolation. Confidential computing utilizes TEEs (Trusted Execution Environments) to remove the cloud provider from the TCB using hardware-based security features. Not all hardware platforms support TEEs, and it thus lack widespread adoption. Furthermore, depending on the type of TEE, there are hardware limitations. For example, in Intel SGX, the enclave size is much less than the total system memory. Intel Xeon E Processors offer a maximum EPC size of 0.5GB
# Solution
This project uses an Intel SGX enclave (emulated)  as a ”key factory” for the Paillier PHE scheme. The enclave will handle sensitive tasks such as key generation, encryption, and decryption, while leaving the computations to untrusted cloud infrastructure. On a high level:
• The enclave securely generates and stores secret keys.
• Client sends ”plaintext” data to the enclave, which encrypts it under the Paillier scheme.
• The encrypted data is outsourced to the untrusted region for computation.
• The untrusted server returns ciphertext results, which are decrypted inside the enclave and sent back to the client in plaintext.

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
