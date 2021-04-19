UNAME_SYSTEM := $(shell uname -s)

GLMPATH = ../glm
CUDAPATH ?= /usr/local/cuda
NVCC = ${CUDAPATH}/bin/nvcc
CXXFLAGS = -std=c++11 -fvisibility=hidden -I$(OFXPATH)/include -I$(BMDOFXDEVPATH)/Support/include -I$(BMDOFXDEVPATH)/OpenFX-1.4/include -I$(GLMPATH)

ifeq ($(UNAME_SYSTEM), Linux)
	BMDOFXDEVPATH = /opt/resolve/Developer
	OPENCLPATH = /opt/AMDAPP
	CXXFLAGS += -I${OPENCLPATH}/include -fPIC
	NVCCFLAGS = --compiler-options="-fPIC"
	LDFLAGS = -shared -fvisibility=hidden -L${CUDAPATH}/lib64 -lcuda -lcudart
	BUNDLE_DIR = Reframe360.ofx.bundle/Contents/Linux-x86-64/
	CUDA_OBJ =  CudaKernel.o
else
	BMDOFXDEVPATH = /Library/Application\ Support/Blackmagic\ Design/DaVinci\ Resolve/Developer/OpenFX
	LDFLAGS = -bundle -fvisibility=hidden -F/Library/Frameworks -framework OpenCL -framework Metal -framework AppKit
	BUNDLE_DIR = Reframe360.ofx.bundle/Contents/MacOS/
	METAL_OBJ = MetalKernel.o
endif

Reframe360.ofx: Reframe360.o OpenCLKernel.o $(CUDA_OBJ) $(METAL_OBJ) ofxsCore.o ofxsImageEffect.o ofxsInteract.o ofxsLog.o ofxsMultiThread.o ofxsParams.o ofxsProperty.o ofxsPropertyValidation.o
	$(CXX) $^ -o $@ $(LDFLAGS)
	mkdir -p $(BUNDLE_DIR)
	cp Reframe360.ofx $(BUNDLE_DIR)

CudaKernel.o: CudaKernel.cu
	${NVCC} -c $< $(NVCCFLAGS)

MetalKernel.o: MetalKernel.mm
	$(CXX) -c $< $(CXXFLAGS)

ofxsCore.o: $(BMDOFXDEVPATH)/Support/Library/ofxsCore.cpp
	$(CXX) -c "$<" $(CXXFLAGS)

ofxsImageEffect.o: $(BMDOFXDEVPATH)/Support/Library/ofxsImageEffect.cpp
	$(CXX) -c "$<" $(CXXFLAGS)

ofxsInteract.o: $(BMDOFXDEVPATH)/Support/Library/ofxsInteract.cpp
	$(CXX) -c "$<" $(CXXFLAGS)

ofxsLog.o: $(BMDOFXDEVPATH)/Support/Library/ofxsLog.cpp
	$(CXX) -c "$<" $(CXXFLAGS)

ofxsMultiThread.o: $(BMDOFXDEVPATH)/Support/Library/ofxsMultiThread.cpp
	$(CXX) -c "$<" $(CXXFLAGS)

ofxsParams.o: $(BMDOFXDEVPATH)/Support/Library/ofxsParams.cpp
	$(CXX) -c "$<" $(CXXFLAGS)

ofxsProperty.o: $(BMDOFXDEVPATH)/Support/Library/ofxsProperty.cpp
	$(CXX) -c "$<" $(CXXFLAGS)

ofxsPropertyValidation.o: $(BMDOFXDEVPATH)/Support/Library/ofxsPropertyValidation.cpp
	$(CXX) -c "$<" $(CXXFLAGS)
	
clean:
	rm -f *.o *.ofx
	rm -fr Reframe360.ofx.bundle

ifeq ($(UNAME_SYSTEM), Darwin)
install:
	rm -rf /Library/OFX/Plugins/Reframe360.ofx.bundle
	cp -a Reframe360.ofx.bundle /Library/OFX/Plugins/
endif
