UNAME_SYSTEM := $(shell uname -s)

GLMPATH = ../glm
CUDAPATH ?= /usr/local/cuda
NVCC = ${CUDAPATH}/bin/nvcc
CXXFLAGS += -std=c++11 -fvisibility=hidden -I$(OFXPATH)/include -I$(BMDOFXDEVPATH)/Support/include -I$(BMDOFXDEVPATH)/OpenFX-1.4/include -I$(GLMPATH)

ifeq ($(UNAME_SYSTEM), Linux)
	BMDOFXDEVPATH = /opt/resolve/Developer
	OPENCLPATH = /opt/AMDAPP
	CXXFLAGS += -I${OPENCLPATH}/include -fPIC -D__OPENCL__
	NVCCFLAGS = --compiler-options="-fPIC"
	LDFLAGS = -shared -fvisibility=hidden -L${CUDAPATH}/lib64 -lcuda -lcudart
	BUNDLE_DIR = Reframe360.ofx.bundle/Contents/Linux-x86-64/
	CUDA_OBJ =  CudaKernel.o
	OPENCL_OBJ = OpenCLKernel.o
else
	BMDOFXDEVPATH = /Library/Application\ Support/Blackmagic\ Design/DaVinci\ Resolve/Developer/OpenFX
	LDFLAGS = -bundle -fvisibility=hidden -F/Library/Frameworks -framework OpenCL -framework Metal -framework AppKit
	BUNDLE_DIR = Reframe360.ofx.bundle/Contents/MacOS/
	METAL_OBJ = Reframe360Kernel.o
	OPENCL_OBJ = OpenCLKernel.o
	METAL_ARM_OBJ = Reframe360Kernel-arm.o
	OPENCL_ARM_OBJ = OpenCLKernel-arm.o
	APPLE86_64_FLAG =  -target x86_64-apple-macos10.12
	APPLEARM64_FLAG =  -target arm64-apple-macos11
endif

Reframe360.ofx:  Reframe360.o $(OPENCL_OBJ) $(CUDA_OBJ) $(METAL_OBJ) ofxsCore.o ofxsImageEffect.o ofxsInteract.o ofxsLog.o ofxsMultiThread.o ofxsParams.o ofxsProperty.o ofxsPropertyValidation.o
	$(CXX) $(APPLE86_64_FLAG) $^ -o $@ $(LDFLAGS)
	mkdir -p $(BUNDLE_DIR)
	cp Reframe360.ofx $(BUNDLE_DIR)

CudaKernel.o: CudaKernel.cu
	${NVCC} -c $< $(NVCCFLAGS)

Reframe360.o: Reframe360.cpp
	$(CXX) $(APPLE86_64_FLAG) -c $< $(CXXFLAGS)
	
Reframe360Kernel.o: Reframe360Kernel.mm
	python metal2string.py Reframe360Kernel.metal Reframe360Kernel.h
	$(CXX) $(APPLE86_64_FLAG) -c $< $(CXXFLAGS)

OpenCLKernel.o: OpenCLKernel.h OpenCLKernel.cpp
	$(CXX) $(APPLE86_64_FLAG) -c OpenCLKernel.cpp $(CXXFLAGS) -o OpenCLKernel.o

OpenCLKernel.h: Reframe360Kernel.cl
	python ./HardcodeKernel.py OpenCLKernel Reframe360Kernel.cl

ofxsCore.o: $(BMDOFXDEVPATH)/Support/Library/ofxsCore.cpp
	$(CXX) $(APPLE86_64_FLAG) -c "$<" $(CXXFLAGS)

ofxsImageEffect.o: $(BMDOFXDEVPATH)/Support/Library/ofxsImageEffect.cpp
	$(CXX) $(APPLE86_64_FLAG) -c "$<" $(CXXFLAGS)

ofxsInteract.o: $(BMDOFXDEVPATH)/Support/Library/ofxsInteract.cpp
	$(CXX) $(APPLE86_64_FLAG) -c "$<" $(CXXFLAGS)

ofxsLog.o: $(BMDOFXDEVPATH)/Support/Library/ofxsLog.cpp
	$(CXX) $(APPLE86_64_FLAG) -c "$<" $(CXXFLAGS)

ofxsMultiThread.o: $(BMDOFXDEVPATH)/Support/Library/ofxsMultiThread.cpp
	$(CXX) $(APPLE86_64_FLAG) -c "$<" $(CXXFLAGS)

ofxsParams.o: $(BMDOFXDEVPATH)/Support/Library/ofxsParams.cpp
	$(CXX) $(APPLE86_64_FLAG) -c "$<" $(CXXFLAGS)

ofxsProperty.o: $(BMDOFXDEVPATH)/Support/Library/ofxsProperty.cpp
	$(CXX) $(APPLE86_64_FLAG) -c "$<" $(CXXFLAGS)

ofxsPropertyValidation.o: $(BMDOFXDEVPATH)/Support/Library/ofxsPropertyValidation.cpp
	$(CXX) $(APPLE86_64_FLAG) -c "$<" $(CXXFLAGS)

Reframe360-arm.ofx:  Reframe360-arm.o $(OPENCL_ARM_OBJ) $(METAL_ARM_OBJ) ofxsCore-arm.o ofxsImageEffect-arm.o ofxsInteract-arm.o ofxsLog-arm.o ofxsMultiThread-arm.o ofxsParams-arm.o ofxsProperty-arm.o ofxsPropertyValidation-arm.o
	$(CXX) $(APPLEARM64_FLAG) $^ -o $@ $(LDFLAGS)
	mkdir -p $(BUNDLE_DIR)
	cp Reframe360.ofx $(BUNDLE_DIR)

Reframe360-arm.o: Reframe360.cpp
	$(CXX) $(APPLEARM64_FLAG) -c $< $(CXXFLAGS) -o $@

Reframe360Kernel-arm.o: Reframe360Kernel.mm
	python metal2string.py Reframe360Kernel.metal Reframe360Kernel.h
	$(CXX) $(APPLEARM64_FLAG) -c $< $(CXXFLAGS) -o $@

OpenCLKernel-arm.o: OpenCLKernel.h OpenCLKernel.cpp
	$(CXX) $(APPLEARM64_FLAG) -c OpenCLKernel.cpp $(CXXFLAGS) -o OpenCLKernel-arm.o
	
ofxsCore-arm.o: $(BMDOFXDEVPATH)/Support/Library/ofxsCore.cpp
	$(CXX) $(APPLEARM64_FLAG) -c "$<" $(CXXFLAGS) -o $@

ofxsImageEffect-arm.o: $(BMDOFXDEVPATH)/Support/Library/ofxsImageEffect.cpp
	$(CXX) $(APPLEARM64_FLAG) -c "$<" $(CXXFLAGS) -o $@

ofxsInteract-arm.o: $(BMDOFXDEVPATH)/Support/Library/ofxsInteract.cpp
	$(CXX) $(APPLEARM64_FLAG) -c "$<" $(CXXFLAGS) -o $@

ofxsLog-arm.o: $(BMDOFXDEVPATH)/Support/Library/ofxsLog.cpp
	$(CXX) $(APPLEARM64_FLAG) -c "$<" $(CXXFLAGS) -o $@

ofxsMultiThread-arm.o: $(BMDOFXDEVPATH)/Support/Library/ofxsMultiThread.cpp
	$(CXX) $(APPLEARM64_FLAG) -c "$<" $(CXXFLAGS) -o $@

ofxsParams-arm.o: $(BMDOFXDEVPATH)/Support/Library/ofxsParams.cpp
	$(CXX) $(APPLEARM64_FLAG) -c "$<" $(CXXFLAGS) -o $@

ofxsProperty-arm.o: $(BMDOFXDEVPATH)/Support/Library/ofxsProperty.cpp
	$(CXX) $(APPLEARM64_FLAG) -c "$<" $(CXXFLAGS) -o $@

ofxsPropertyValidation-arm.o: $(BMDOFXDEVPATH)/Support/Library/ofxsPropertyValidation.cpp
	$(CXX) $(APPLEARM64_FLAG) -c "$<" $(CXXFLAGS) -o $@

Reframe360Kernel.h: Reframe360Kernel.metal
	python metal2string.py Reframe360Kernel.metal Reframe360Kernel.h

%.metallib: %.metal
	xcrun -sdk macosx metal -c $< -o $@
	mkdir -p $(BUNDLE_DIR)
	cp $@ $(BUNDLE_DIR)

macos-bin: install-universal
	zip -r Reframe360.ofx.bundle.zip Reframe360.ofx.bundle

clean:
	rm -f *.o *.ofx *.zip *.metallib Reframe360Kernel.h OpenCLKernel.h
	rm -fr Reframe360.ofx.bundle

ifeq ($(UNAME_SYSTEM), Darwin)
.DEFAULT_GOAL := darwin

.PHONY: darwin
darwin: clean install-universal macos-bin

install: Reframe360.ofx
	rm -rf /Library/OFX/Plugins/Reframe360.ofx.bundle
	cp -a Reframe360.ofx.bundle /Library/OFX/Plugins/

install-universal: Reframe360.ofx Reframe360-arm.ofx
	lipo -create -output Reframe360-universal.ofx Reframe360.ofx Reframe360-arm.ofx
	rm -rf $(BUNDLE_DIR)
	mkdir -p $(BUNDLE_DIR)
	cp Reframe360-universal.ofx $(BUNDLE_DIR)/Reframe360.ofx
	rm -rf /Library/OFX/Plugins/Reframe360.ofx.bundle
	cp -a Reframe360.ofx.bundle /Library/OFX/Plugins/
endif
