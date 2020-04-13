UNAME_SYSTEM := $(shell uname -s)

CUDAPATH ?= /usr/local/cuda
NVCC = ${CUDAPATH}/bin/nvcc
CXXFLAGS = -fvisibility=hidden -I../OpenFX-1.4/include -I../Support/include

ifeq ($(UNAME_SYSTEM), Linux)
	AMDAPP_PATH ?= /opt/AMDAPP
	CXXFLAGS += -I${AMDAPP_PATH}/include -fPIC
	NVCCFLAGS = --compiler-options="-fPIC"
	LDFLAGS = -shared -fvisibility=hidden -L${CUDAPATH}/lib64 -lcuda -lcudart_static
	BUNDLE_DIR = Reframe360.ofx.bundle/Contents/Linux-x86-64/
else
	LDFLAGS = -bundle -fvisibility=hidden -L${CUDAPATH}/lib -lcudart_static -F/Library/Frameworks -framework OpenCL -framework Metal -framework AppKit
	BUNDLE_DIR = Reframe360.ofx.bundle/Contents/MacOS/
	METAL_OBJ = MetalKernel.o
endif

Reframe360.ofx: Reframe360.o CudaKernel.o $(METAL_OBJ) OpenCLKernel.o ofxsCore.o ofxsImageEffect.o ofxsInteract.o ofxsLog.o ofxsMultiThread.o ofxsParams.o ofxsProperty.o ofxsPropertyValidation.o
	$(CXX) $^ -o $@ $(LDFLAGS)
	mkdir -p $(BUNDLE_DIR)
	cp Reframe360.ofx $(BUNDLE_DIR)

CudaKernel.o: CudaKernel.cu
	${NVCC} -c $< $(NVCCFLAGS)

MetalKernel.o: MetalKernel.mm
	$(CXX) -c $< $(CXXFLAGS)

%.o: ../Support/Library/%.cpp
	$(CXX) -c $< $(CXXFLAGS)
	
clean:
	rm -f *.o *.ofx
	rm -fr Reframe360.ofx.bundle