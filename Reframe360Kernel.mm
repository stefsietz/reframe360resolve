#import <Metal/Metal.h>
#import "Reframe360Kernel.h"

void RunMetalKernel(void* p_CmdQ, int p_Width, int p_Height, float* p_Fov, float* p_Tinyplanet, float* p_Rectilinear, const float* p_Input, float* p_Output,float* p_RotMat, int p_Samples,
                            bool p_Bilinear)
{
    const char* kernelName = "Reframe360Kernel";

    id<MTLDevice>                  device;
    id<MTLCommandQueue>            queue;
    id<MTLLibrary>                 metalLibrary;     // Metal library
    id<MTLFunction>                kernelFunction;   // Compute kernel
    id<MTLComputePipelineState>    pipelineState;    // Metal pipeline
    NSError* err;

    if (!(device = MTLCreateSystemDefaultDevice()))
    {
    	fprintf(stderr, "Metal is not supported on this device\n");
    	return;
    }
    if (!(queue = [device newCommandQueueWithMaxCommandBufferCount:1]))
    {
    	fprintf(stderr, "Unable to resever queue with max command buffer count\n");
    }

    MTLCompileOptions* options = [MTLCompileOptions new];
    options.fastMathEnabled = YES;
    if (!(metalLibrary    = [device newLibraryWithSource:@(metal_src_Reframe360Kernel) options:options error:&err]))
    {
        fprintf(stderr, "Failed to load metal library, %s\n", err.localizedDescription.UTF8String);
        return;
    }
    [options release];
    if (!(kernelFunction  = [metalLibrary newFunctionWithName:[NSString stringWithUTF8String:kernelName]/* constantValues : constantValues */]))
    {
        fprintf(stderr, "Failed to retrive kernel\n");
        [metalLibrary release];
        return;
    }
    if (!(pipelineState   = [device newComputePipelineStateWithFunction:kernelFunction error:&err]))
    {
        fprintf(stderr, "Unable to compile, %s\n", err.localizedDescription.UTF8String);
        [metalLibrary release];
        [kernelFunction release];
        return;
    }

    id<MTLBuffer> srcDeviceBuf = reinterpret_cast<id<MTLBuffer> >(const_cast<float *>(p_Input));
    id<MTLBuffer> dstDeviceBuf = reinterpret_cast<id<MTLBuffer> >(p_Output);

    id<MTLBuffer> srcFovDeviceBuf = reinterpret_cast<id<MTLBuffer> >(const_cast<float *>(p_Fov));
    id<MTLBuffer> srcRotMatDeviceBuf = reinterpret_cast<id<MTLBuffer> >(const_cast<float *>(p_RotMat));
    id<MTLBuffer> srcTinyplanetDeviceBuf = reinterpret_cast<id<MTLBuffer> >(const_cast<float *>(p_Tinyplanet));
    id<MTLBuffer> srcRectilinearDeviceBuf = reinterpret_cast<id<MTLBuffer> >(const_cast<float *>(p_Rectilinear));
    
    id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];
    commandBuffer.label = [NSString stringWithFormat:@"Reframe360Kernel"];

    id<MTLComputeCommandEncoder> computeEncoder = [commandBuffer computeCommandEncoder];
    [computeEncoder setComputePipelineState:pipelineState];

    int exeWidth = [pipelineState threadExecutionWidth];
    MTLSize threadGroupCount = MTLSizeMake(exeWidth, 1, 1);
    MTLSize threadGroups     = MTLSizeMake((p_Width + exeWidth - 1)/exeWidth, p_Height, 1);
#ifdef DEBUG
    fprintf(stdout, "MetalKernel Working for, W:%d H:%d\n\trotMatrix: %2.2f\t%2.2f\t%2.2f\n\t         : %2.2f\t%2.2f\t%2.2f\n\t         : %2.2f\t%2.2f\t%2.2f\n\tfov:%2.6f tinyplanet:%2.6f rectilinear:%2.6f samples:%d bilinear:%d \n", p_Width, p_Height, p_RotMat[0],p_RotMat[1],p_RotMat[2],p_RotMat[3],p_RotMat[4],p_RotMat[5],p_RotMat[6],p_RotMat[7],p_RotMat[8],p_Fov[0], p_Tinyplanet[0], p_Rectilinear[0], p_Samples,p_Bilinear);
#endif
    [computeEncoder setBuffer:srcDeviceBuf offset: 0 atIndex: 0];
    [computeEncoder setBuffer:dstDeviceBuf offset: 0 atIndex: 8];
    [computeEncoder setBytes:&p_Width length:sizeof(int) atIndex:11];
    [computeEncoder setBytes:&p_Height length:sizeof(int) atIndex:12];
    [computeEncoder setBytes:p_Fov length:(sizeof(float)*p_Samples) atIndex:13];
    [computeEncoder setBytes:p_Tinyplanet length:(sizeof(float)*p_Samples) atIndex:14];
    [computeEncoder setBytes:p_Rectilinear length:(sizeof(float)*p_Samples) atIndex:15];
    [computeEncoder setBytes:p_RotMat length:(sizeof(float[9])*p_Samples) atIndex:16];
    [computeEncoder setBytes:&p_Samples length:sizeof(int) atIndex:17];
    [computeEncoder setBytes:&p_Bilinear length:sizeof(bool) atIndex:18];
    [computeEncoder dispatchThreadgroups:threadGroups threadsPerThreadgroup: threadGroupCount];

    [computeEncoder endEncoding];
    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];

    //Release resources
    [metalLibrary release];
    [kernelFunction release];
    [pipelineState release];
    [queue release];
    [device release];
}
