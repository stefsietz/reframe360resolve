UNAME_SYSTEM := $(shell uname -s)

CUDAPATH ?= /usr/local/cuda
NVCC = ${CUDAPATH}/bin/nvcc
CXXFLAGS = -fvisibility=hidden -I../OpenFX-1.3/include -I../Support/include

ifeq ($(UNAME_SYSTEM), Linux)
	OPENCLPATH = /opt/AMDAPP
	CXXFLAGS += -I${OPENCLPATH}/include -fPIC
	NVCCFLAGS = --compiler-options="-fPIC"
	LDFLAGS = -shared -fvisibility=hidden -L${CUDAPATH}/lib64 -lcuda -lcudart
	BUNDLE_DIR = GainPlugin.ofx.bundle/Contents/Linux-x86-64/
else
	LDFLAGS = -bundle -fvisibility=hidden -L${CUDAPATH}/lib -lcudart -F/Library/Frameworks -framework OpenCL -framework Metal -framework AppKit
	BUNDLE_DIR = GainPlugin.ofx.bundle/Contents/MacOS/
	METAL_OBJ = MetalKernel.o
endif

GainPlugin.ofx: GainPlugin.o CudaKernel.o $(METAL_OBJ) OpenCLKernel.o ofxsCore.o ofxsImageEffect.o ofxsInteract.o ofxsLog.o ofxsMultiThread.o ofxsParams.o ofxsProperty.o ofxsPropertyValidation.o
	$(CXX) $^ -o $@ $(LDFLAGS)
	mkdir -p $(BUNDLE_DIR)
	cp GainPlugin.ofx $(BUNDLE_DIR)

CudaKernel.o: CudaKernel.cu
	${NVCC} -c $< $(NVCCFLAGS)

MetalKernel.o: MetalKernel.mm
	$(CXX) -c $< $(CXXFLAGS)

%.o: ../Support/Library/%.cpp
	$(CXX) -c $< $(CXXFLAGS)
	
clean:
	rm -f *.o *.ofx
	rm -fr GainPlugin.ofx.bundle
