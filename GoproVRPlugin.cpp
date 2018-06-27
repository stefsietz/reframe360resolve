#include "GoproVRPlugin.h"

#include <stdio.h>


#include "ofxsImageEffect.h"
#include "ofxsMultiThread.h"
#include "ofxsProcessing.h"
#include "ofxsLog.h"

#define kPluginName "GoproVR"
#define kPluginGrouping "OpenFX Sample"
#define kPluginDescription "Apply seperate RGB gain adjustments to each channels"
#define kPluginIdentifier "com.blackmagicdesign.OpenFXSample.GoproVRPlugin"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

#define kSupportsTiles false
#define kSupportsMultiResolution false
#define kSupportsMultipleClipPARs false

#define M_PI 3.14159265358979323846

///GLM Imports

#include "MathUtil.h"

////////////////////////////////////////////////////////////////////////////////

class ImageScaler : public OFX::ImageProcessor
{
public:
    explicit ImageScaler(OFX::ImageEffect& p_Instance);

    virtual void processImagesCUDA();
    virtual void processImagesOpenCL();
#if defined(__APPLE__)
    virtual void processImagesMetal();
#endif
    virtual void multiThreadProcessImages(OfxRectI p_ProcWindow);

    void setSrcImg(OFX::Image* p_SrcImg);
    void setParams(float* p_RotMat, float* p_Fov, float* p_Fisheye, int p_Samples, bool p_Bilinear);

private:
    OFX::Image* _srcImg;
    float* _rotMat;
	float* _fov;
	float* _fisheye;
	int _samples;
	bool _bilinear;
};

ImageScaler::ImageScaler(OFX::ImageEffect& p_Instance)
    : OFX::ImageProcessor(p_Instance)
{
}

void pitchMatrix(float pitch, float** out) {
		(*out)[0] = 1.0;
		(*out)[1] = 0;
		(*out)[2] = 0;
		(*out)[3] = 0;
		(*out)[4] = cos(pitch);
		(*out)[5] = -sin(pitch);
		(*out)[6] = 0;
		(*out)[7] = sin(pitch);
		(*out)[8] = cos(pitch);
}

void yawMatrix(float yaw, float** out) {
	(*out)[0] = cos(yaw);
	(*out)[1] = 0;
	(*out)[2] = sin(yaw);
	(*out)[3] = 0;
	(*out)[4] = 1.0;
	(*out)[5] = 0;
	(*out)[6] = -sin(yaw);
	(*out)[7] = 0;
	(*out)[8] = cos(yaw);
}

void matMul(const float* y, const float* p, float** outmat){
	(*outmat)[0] = p[0] * y[0] + p[3] * y[1] + p[6] * y[2];
	(*outmat)[1] = p[1] * y[0] + p[4] * y[1] + p[7] * y[2];
	(*outmat)[2] = p[3] * y[0] + p[5] * y[1] + p[8] * y[2];
	(*outmat)[3] = p[0] * y[3] + p[3] * y[4] + p[6] * y[5];
	(*outmat)[4] = p[1] * y[3] + p[4] * y[4] + p[7] * y[5];
	(*outmat)[5] = p[3] * y[3] + p[5] * y[4] + p[8] * y[5];
	(*outmat)[6] = p[0] * y[6] + p[3] * y[7] + p[6] * y[8];
	(*outmat)[7] = p[1] * y[6] + p[4] * y[7] + p[7] * y[8];
	(*outmat)[8] = p[3] * y[6] + p[5] * y[7] + p[8] * y[8];
}

extern void RunCudaKernel(int p_Width, int p_Height, float* p_Fov, float* p_Fisheye, const float* p_Input, float* p_Output, const float* p_RotMat, int p_Samples, bool p_Bilinear);

void ImageScaler::processImagesCUDA()
{
    const OfxRectI& bounds = _srcImg->getBounds();
    const int width = bounds.x2 - bounds.x1;
    const int height = bounds.y2 - bounds.y1;

    float* input = static_cast<float*>(_srcImg->getPixelData());
    float* output = static_cast<float*>(_dstImg->getPixelData());

	RunCudaKernel(width, height, _fov, _fisheye, input, output, _rotMat, _samples, _bilinear);
}

#if defined(__APPLE__)
extern void RunMetalKernel(int p_Width, int p_Height, float* p_Gain, const float* p_Input, float* p_Output);

void ImageScaler::processImagesMetal()
{
    const OfxRectI& bounds = _srcImg->getBounds();
    const int width = bounds.x2 - bounds.x1;
    const int height = bounds.y2 - bounds.y1;

    float* input = static_cast<float*>(_srcImg->getPixelData());
    float* output = static_cast<float*>(_dstImg->getPixelData());

    RunMetalKernel(width, height, _params, input, output);
}
#endif

extern void RunOpenCLKernel(void* p_CmdQ, int p_Width, int p_Height, float* p_Fov, float* p_Fisheye, const float* p_Input, float* p_Output, float* p_RotMat, int p_Samples, bool p_Bilinear);

void ImageScaler::processImagesOpenCL()
{
    const OfxRectI& bounds = _srcImg->getBounds();
    const int width = bounds.x2 - bounds.x1;
    const int height = bounds.y2 - bounds.y1;

    float* input = static_cast<float*>(_srcImg->getPixelData());
	float* output = static_cast<float*>(_dstImg->getPixelData());

	RunOpenCLKernel(_pOpenCLCmdQ, width, height, _fov, _fisheye, input, output, _rotMat, _samples, _bilinear);
}

void ImageScaler::multiThreadProcessImages(OfxRectI p_ProcWindow)
{
	int width = p_ProcWindow.x2 - p_ProcWindow.x1;
	int height = p_ProcWindow.y2 - p_ProcWindow.y1;

	for (int i = 0; i < _samples; i++)
	{

		mat3 rotMat;
		rotMat[0][0] = _rotMat[i * 9 + 0];
		rotMat[0][1] = _rotMat[i * 9 + 1];
		rotMat[0][2] = _rotMat[i * 9 + 2];
		rotMat[1][0] = _rotMat[i * 9 + 3];
		rotMat[1][1] = _rotMat[i * 9 + 4];
		rotMat[1][2] = _rotMat[i * 9 + 5];
		rotMat[2][0] = _rotMat[i * 9 + 6];
		rotMat[2][1] = _rotMat[i * 9 + 7];
		rotMat[2][2] = _rotMat[i * 9 + 8];

		float fov = _fov[i];
		float aspect = (float)width / (float)height;

		for (int y = p_ProcWindow.y1; y < p_ProcWindow.y2; ++y)
		{
			if (_effect.abort()) break;

			float* dstPix = static_cast<float*>(_dstImg->getPixelAddress(p_ProcWindow.x1, y));

			for (int x = p_ProcWindow.x1; x < p_ProcWindow.x2; ++x)
			{
				vec2 uv = { (float)x / width, (float)y / height };

				vec3 dir = { 0, 0, 0 };
				dir.x = (uv.x - 0.5)*2.0;
				dir.y = (uv.y - 0.5)*2.0;
				dir.y /= aspect;
				dir.z = fov;

				vec3 rectdir = rotMat * dir;

				rectdir = normalize(rectdir);

				dir = mix(rectdir, fisheyeDir(dir, rotMat), _fisheye[i]);

				vec2 iuv = polarCoord(dir);
				iuv = repairUv(iuv);

				int x_new = p_ProcWindow.x1 + iuv.x * (width - 1);
				int y_new = p_ProcWindow.y1 + iuv.y * (height - 1);

				iuv.x *= (width - 1);
				iuv.y *= (height - 1);

				if ((x_new < width) && (y_new < height))
				{
					float* srcPix = static_cast<float*>(_srcImg ? _srcImg->getPixelAddress(x_new, y_new) : 0);
					vec4 interpCol;

					// do we have a source image to scale up
					if (srcPix)
					{
						if (_bilinear){
							interpCol = linInterpCol(iuv, _srcImg, p_ProcWindow, width, height);
						}
						else {
							interpCol = { srcPix[0], srcPix[1], srcPix[2], srcPix[3] };
						}
					}
					else
					{
						interpCol = { 0, 0, 0, 1.0 };
					}

					if (i == 0) {
						dstPix[0] = 0;
						dstPix[1] = 0;
						dstPix[2] = 0;
						dstPix[3] = 0;
					}

					dstPix[0] += interpCol.x / _samples;
					dstPix[1] += interpCol.y / _samples;
					dstPix[2] += interpCol.z / _samples;
					dstPix[3] += interpCol.w / _samples;
				}

				// increment the dst pixel
				dstPix += 4;

				continue;
			}
		}
	}
}

void ImageScaler::setSrcImg(OFX::Image* p_SrcImg)
{
    _srcImg = p_SrcImg;
}

void ImageScaler::setParams(float* p_RotMat, float* p_Fov, float* p_Fisheye, int p_Samples, bool p_Bilinear)
{
	_rotMat = p_RotMat;
	_fov = p_Fov;
    _fisheye = p_Fisheye;
    _samples = p_Samples;
	_bilinear = p_Bilinear;
}

////////////////////////////////////////////////////////////////////////////////
/** @brief The plugin that does our work */
class GainPlugin : public OFX::ImageEffect
{
public:
    explicit GainPlugin(OfxImageEffectHandle p_Handle);

    /* Override the render */
    virtual void render(const OFX::RenderArguments& p_Args);

    /* Override is identity */
    virtual bool isIdentity(const OFX::IsIdentityArguments& p_Args, OFX::Clip*& p_IdentityClip, double& p_IdentityTime);

    /* Override changedParam */
    virtual void changedParam(const OFX::InstanceChangedArgs& p_Args, const std::string& p_ParamName);

    /* Override changed clip */
    virtual void changedClip(const OFX::InstanceChangedArgs& p_Args, const std::string& p_ClipName);

    /* Set the enabledness of the component scale params depending on the type of input image and the state of the scaleComponents param */
    void setEnabledness();

    /* Set up and run a processor */
    void setupAndProcess(ImageScaler &p_ImageScaler, const OFX::RenderArguments& p_Args);

private:
    // Does not own the following pointers
    OFX::Clip* m_DstClip;
    OFX::Clip* m_SrcClip;

    OFX::DoubleParam* m_Pitch;
    OFX::DoubleParam* m_Yaw;
    OFX::DoubleParam* m_Fov;
    OFX::DoubleParam* m_Fisheye;

	OFX::DoubleParam* m_Blend;
	OFX::DoubleParam* m_Accel;

	OFX::DoubleParam* m_Shutter;
	OFX::IntParam* m_Samples;
	OFX::BooleanParam* m_Bilinear;

	OFX::DoubleParam* m_Pitch2;
	OFX::DoubleParam* m_Yaw2;
	OFX::DoubleParam* m_Fov2;
	OFX::DoubleParam* m_Fisheye2;
};

GainPlugin::GainPlugin(OfxImageEffectHandle p_Handle)
    : ImageEffect(p_Handle)
{
    m_DstClip = fetchClip(kOfxImageEffectOutputClipName);
    m_SrcClip = fetchClip(kOfxImageEffectSimpleSourceClipName);

	m_Pitch =  fetchDoubleParam("pitch");
    m_Yaw = fetchDoubleParam("yaw");
    m_Fov = fetchDoubleParam("fov");
    m_Fisheye = fetchDoubleParam("fisheye");

	m_Blend = fetchDoubleParam("blend");
	m_Accel = fetchDoubleParam("accel");

	m_Shutter = fetchDoubleParam("shutter");
	m_Samples = fetchIntParam("samples");
	m_Bilinear = fetchBooleanParam("bilinear");

	m_Pitch2 = fetchDoubleParam("pitch2");
	m_Yaw2 = fetchDoubleParam("yaw2");
	m_Fov2 = fetchDoubleParam("fov2");
	m_Fisheye2 = fetchDoubleParam("fisheye2");

    // Set the enabledness of our RGBA sliders
    setEnabledness();
}

void GainPlugin::render(const OFX::RenderArguments& p_Args)
{
    if ((m_DstClip->getPixelDepth() == OFX::eBitDepthFloat) && (m_DstClip->getPixelComponents() == OFX::ePixelComponentRGBA))
    {
        ImageScaler imageScaler(*this);
        setupAndProcess(imageScaler, p_Args);
    }
    else
    {
        OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
    }
}

bool GainPlugin::isIdentity(const OFX::IsIdentityArguments& p_Args, OFX::Clip*& p_IdentityClip, double& p_IdentityTime)
{
    return false;
}

void GainPlugin::changedParam(const OFX::InstanceChangedArgs& p_Args, const std::string& p_ParamName)
{

    //setEnabledness();

}

void GainPlugin::changedClip(const OFX::InstanceChangedArgs& p_Args, const std::string& p_ClipName)
{
    if (p_ClipName == kOfxImageEffectSimpleSourceClipName)
    {
        setEnabledness();
    }
}

void GainPlugin::setEnabledness()
{
    // the component enabledness depends on the clip being RGBA and the param being true
    const bool enable = ((m_SrcClip->getPixelComponents() == OFX::ePixelComponentRGBA));

    m_Pitch->setEnabled(enable);
    m_Yaw->setEnabled(enable);
    m_Fov->setEnabled(enable);
    m_Fisheye->setEnabled(enable);

	m_Blend->setEnabled(enable);
	m_Accel->setEnabled(enable);

	m_Pitch2->setEnabled(enable);
	m_Yaw2->setEnabled(enable);
	m_Fov2->setEnabled(enable);
	m_Fisheye2->setEnabled(enable);
}

static float fitRange(float value, float in_min, float in_max, float out_min, float out_max){
	float out = out_min + ((out_max - out_min) / (in_max - in_min)) * (value - in_min);
	return std::min(out_max, std::max(out, out_min));
}

static float interpParam(OFX::DoubleParam* param, const OFX::RenderArguments& p_Args, float offset){
	if (offset == 0){
		return param->getValueAtTime(p_Args.time);
	}
	else if (offset < 0) {
		offset = -offset;
		float floor = std::floor(offset);
		float frac = offset - floor;

		return param->getValueAtTime(p_Args.time - (floor + 1))*frac + param->getValueAtTime(p_Args.time - floor)*(1 - frac);
	}
	else {
		float floor = std::floor(offset);
		float frac = offset - floor;

		return param->getValueAtTime(p_Args.time + (floor + 1))*frac + param->getValueAtTime(p_Args.time + floor)*(1 - frac);
	}
}

void GainPlugin::setupAndProcess(ImageScaler& p_ImageScaler, const OFX::RenderArguments& p_Args)
{
    // Get the dst image
    std::auto_ptr<OFX::Image> dst(m_DstClip->fetchImage(p_Args.time));
    OFX::BitDepthEnum dstBitDepth = dst->getPixelDepth();
    OFX::PixelComponentEnum dstComponents = dst->getPixelComponents();

    // Get the src image
    std::auto_ptr<OFX::Image> src(m_SrcClip->fetchImage(p_Args.time));
    OFX::BitDepthEnum srcBitDepth = src->getPixelDepth();
    OFX::PixelComponentEnum srcComponents = src->getPixelComponents();

    // Check to see if the bit depth and number of components are the same
    if ((srcBitDepth != dstBitDepth) || (srcComponents != dstComponents))
    {
        OFX::throwSuiteStatusException(kOfxStatErrValue);
    }

	double pitch = 1.0, yaw = 1.0, fov = 1.0, fisheye = 1.0, pitch2 = 1.0, yaw2 = 1.0, fov2 = 1.0, fisheye2 = 1.0, blend = 0.0, accel = 0.5;

	int mb_samples = (int)m_Samples->getValueAtTime(p_Args.time);
	float mb_shutter = m_Shutter->getValueAtTime(p_Args.time) * 0.5;
	int bilinear = m_Bilinear->getValueAtTime(p_Args.time);

	float* fovs = (float*)malloc(sizeof(float)*mb_samples);
	float* fisheyes = (float*)malloc(sizeof(float)*mb_samples);
	float* rotmats = (float*)malloc(sizeof(float)*mb_samples*9);

	for (int i = 0; i < mb_samples; i++){
		float offset = 0;
		if (mb_samples > 1){
			offset = fitRange((float)i, 0, mb_samples - 1.0f, -1.0f, 1.0f);
		}

		offset *= mb_shutter;

		pitch = interpParam(m_Pitch, p_Args, offset);
		yaw = interpParam(m_Yaw, p_Args, offset);
		fov = interpParam(m_Fov, p_Args, offset);
		fisheye = interpParam(m_Fisheye, p_Args, offset);

		blend = interpParam(m_Blend, p_Args, offset);
		accel = interpParam(m_Accel, p_Args, offset);

		pitch2 = interpParam(m_Pitch2, p_Args, offset);
		yaw2 = interpParam(m_Yaw2, p_Args, offset);
		fov2 = interpParam(m_Fov2, p_Args, offset);
		fisheye2 = interpParam(m_Fisheye2, p_Args, offset);

		if (blend < 0.5){
			blend = fitRange(blend, 0, 0.5, 0, 1);
			blend = std::pow(blend, accel);
			blend = fitRange(blend, 0, 1, 0, 0.5);
		}
		else{
			blend = fitRange(blend, 0.5, 1.0, 0, 1);
			blend = 1.0 - blend;
			blend = std::pow(blend, accel);
			blend = 1.0 - blend;
			blend = fitRange(blend, 0, 1, 0.5, 1.0);
		}

		pitch = pitch * (1.0 - blend) + pitch2 * blend;
		yaw = yaw * (1.0 - blend) + yaw2 * blend;
		fov = fov * (1.0 - blend) + fov2 * blend;
		fisheye = fisheye * (1.0 - blend) + fisheye2 * blend;

		float* pitchMat = (float*)calloc(9, sizeof(float));
		pitchMatrix(-pitch / 180 * M_PI, &pitchMat);
		float* yawMat = (float*)calloc(9, sizeof(float));
		yawMatrix(yaw / 180 * M_PI, &yawMat);
		float* rotMat = (float*)calloc(9, sizeof(float));
		matMul(yawMat, pitchMat, &rotMat);

		free(pitchMat);
		free(yawMat);

		memcpy(&(rotmats[i * 9]), rotMat, sizeof(float) * 9);
		free(rotMat);

		fovs[i] = fov;
		fisheyes[i] = fisheye;
	}
    
    // Set the images
    p_ImageScaler.setDstImg(dst.get());
    p_ImageScaler.setSrcImg(src.get());

    // Setup OpenCL and CUDA Render arguments
    p_ImageScaler.setGPURenderArgs(p_Args);

    // Set the render window
    p_ImageScaler.setRenderWindow(p_Args.renderWindow);

    // Set the scales
	p_ImageScaler.setParams(rotmats, fovs, fisheyes, mb_samples, bilinear);

    // Call the base class process member, this will call the derived templated process code
    p_ImageScaler.process();
}

////////////////////////////////////////////////////////////////////////////////

using namespace OFX;

GainPluginFactory::GainPluginFactory()
    : OFX::PluginFactoryHelper<GainPluginFactory>(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor)
{
}

void GainPluginFactory::describe(OFX::ImageEffectDescriptor& p_Desc)
{
    // Basic labels
    p_Desc.setLabels(kPluginName, kPluginName, kPluginName);
    p_Desc.setPluginGrouping(kPluginGrouping);
    p_Desc.setPluginDescription(kPluginDescription);

    // Add the supported contexts, only filter at the moment
    p_Desc.addSupportedContext(eContextFilter);
    p_Desc.addSupportedContext(eContextGeneral);

    // Add supported pixel depths
    p_Desc.addSupportedBitDepth(eBitDepthFloat);

    // Set a few flags
    p_Desc.setSingleInstance(false);
    p_Desc.setHostFrameThreading(false);
    p_Desc.setSupportsMultiResolution(kSupportsMultiResolution);
    p_Desc.setSupportsTiles(kSupportsTiles);
    p_Desc.setTemporalClipAccess(false);
    p_Desc.setRenderTwiceAlways(false);
    p_Desc.setSupportsMultipleClipPARs(kSupportsMultipleClipPARs);

    // Setup OpenCL and CUDA render capability flags
    p_Desc.setSupportsOpenCLRender(true);
    p_Desc.setSupportsCudaRender(true);
    p_Desc.setSupportsMetalRender(false);
}

static DoubleParamDescriptor* defineParam(OFX::ImageEffectDescriptor& p_Desc, const std::string& p_Name, const std::string& p_Label,
	const std::string& p_Hint, GroupParamDescriptor* p_Parent, float min, float max, float default, float hardmin = INT_MIN, float hardmax = INT_MAX)
{
    DoubleParamDescriptor* param = p_Desc.defineDoubleParam(p_Name);
    param->setLabels(p_Label, p_Label, p_Label);
    param->setScriptName(p_Name);
    param->setHint(p_Hint);
    param->setDefault(default);
	param->setRange(hardmin, hardmax);
    param->setIncrement(0.1);
    param->setDisplayRange(min, max);
    param->setDoubleType(eDoubleTypePlain);

    if (p_Parent)
    {
        param->setParent(*p_Parent);
    }

    return param;
}

static IntParamDescriptor* defineIntParam(OFX::ImageEffectDescriptor& p_Desc, const std::string& p_Name, const std::string& p_Label,
	const std::string& p_Hint, GroupParamDescriptor* p_Parent, int min, int max, int default, int hardmin = INT_MIN, int hardmax = INT_MAX)
{
	IntParamDescriptor* param = p_Desc.defineIntParam(p_Name);
	param->setLabels(p_Label, p_Label, p_Label);
	param->setScriptName(p_Name);
	param->setHint(p_Hint);
	param->setDefault(default);
	param->setRange(hardmin, hardmax);
	param->setDisplayRange(min, max);

	if (p_Parent)
	{
		param->setParent(*p_Parent);
	}

	return param;
}

static BooleanParamDescriptor* defineBooleanParam(OFX::ImageEffectDescriptor& p_Desc, const std::string& p_Name, const std::string& p_Label,
	const std::string& p_Hint, GroupParamDescriptor* p_Parent, bool default)
{
	BooleanParamDescriptor* param = p_Desc.defineBooleanParam(p_Name);
	param->setLabels(p_Label, p_Label, p_Label);
	param->setScriptName(p_Name);
	param->setHint(p_Hint);
	param->setDefault(default);

	if (p_Parent)
	{
		param->setParent(*p_Parent);
	}

	return param;
}


void GainPluginFactory::describeInContext(OFX::ImageEffectDescriptor& p_Desc, OFX::ContextEnum /*p_Context*/)
{
    // Source clip only in the filter context
    // Create the mandated source clip
    ClipDescriptor* srcClip = p_Desc.defineClip(kOfxImageEffectSimpleSourceClipName);
    srcClip->addSupportedComponent(ePixelComponentRGBA);
    srcClip->setTemporalClipAccess(false);
    srcClip->setSupportsTiles(kSupportsTiles);
    srcClip->setIsMask(false);

    // Create the mandated output clip
    ClipDescriptor* dstClip = p_Desc.defineClip(kOfxImageEffectOutputClipName);
    dstClip->addSupportedComponent(ePixelComponentRGBA);
    dstClip->addSupportedComponent(ePixelComponentAlpha);
    dstClip->setSupportsTiles(kSupportsTiles);

    // Make some pages and to things in
    PageParamDescriptor* page = p_Desc.definePageParam("Controls");

    GroupParamDescriptor* camera1ParamsGroup = p_Desc.defineGroupParam("camera1Params");
    camera1ParamsGroup->setHint("Scales on the individual component");
    camera1ParamsGroup->setLabels("Camera 1 Parameters", "Camera 1 Parameters", "Camera 1 Parameters");


    // Make the camera 1 params
	DoubleParamDescriptor* param = defineParam(p_Desc, "pitch", "Pitch", "Up/down camera rotation", camera1ParamsGroup, -90, 90, 0);
    page->addChild(*param);

    param = defineParam(p_Desc, "yaw", "Yaw", "Left/right camera rotation", camera1ParamsGroup, -180, 180, 0);
    page->addChild(*param);

    param = defineParam(p_Desc, "fov", "Field of View", "Camera field of view", camera1ParamsGroup, 0.15, 5, 1);
    page->addChild(*param);

    param = defineParam(p_Desc, "fisheye", "Fisheye", "Degree of fisheye distortion", camera1ParamsGroup, 0, 1, 0, 0, 1);
    page->addChild(*param);

	GroupParamDescriptor* camera2ParamsGroup = p_Desc.defineGroupParam("camera2Params");
	camera2ParamsGroup->setHint("Scales on the individual component");
	camera2ParamsGroup->setLabels("Camera 2 Parameters", "Camera 2 Parameters", "Camera 2 Parameters");

	// Make the camera 2 params
	param = defineParam(p_Desc, "pitch2", "Pitch", "Up/down camera rotation", camera2ParamsGroup, -90, 90, 0);
	page->addChild(*param);

	param = defineParam(p_Desc, "yaw2", "Yaw", "Left/right camera rotation", camera2ParamsGroup, -180, 180, 0);
	page->addChild(*param);

	param = defineParam(p_Desc, "fov2", "Field of View", "Camera field of view", camera2ParamsGroup, 0.15, 5, 1);
	page->addChild(*param);

	param = defineParam(p_Desc, "fisheye2", "Fisheye", "Degree of fisheye distortion", camera2ParamsGroup, 0, 1, 0, 0, 1);
	page->addChild(*param);

	GroupParamDescriptor* blendGroup = p_Desc.defineGroupParam("blendParams");
	blendGroup->setHint("Blend Camera Parameters");
	blendGroup->setLabels("Blend Camera Parameters", "Blend Camera Parameters", "Blend Camera Parameters");

	param = defineParam(p_Desc, "blend", "Blend", "Blend Cameras", blendGroup, 0, 1, 0);
	page->addChild(*param);

	param = defineParam(p_Desc, "accel", "Blend Acceleration", "Blend Acceleration", blendGroup, 0, 1, 0);
	page->addChild(*param);

	GroupParamDescriptor* motionblurGroup = p_Desc.defineGroupParam("motionblurParams");
	blendGroup->setHint("Blend Camera Parameters");
	blendGroup->setLabels("Motion Blur Parameters", "Motion Blur Parameters", "Motion Blur Parameters");

	param = defineParam(p_Desc, "shutter", "Shutter Angle", "Shutter Angle", motionblurGroup, 0, 1, 0, 0, 3);
	page->addChild(*param);

	IntParamDescriptor* intParam = defineIntParam(p_Desc, "samples", "Samples", "Samples", motionblurGroup, 1, 20, 1, 1, 256);
	page->addChild(*intParam);

	BooleanParamDescriptor* bilinearParam = defineBooleanParam(p_Desc, "bilinear", "Bilinear Filtering", "Bilinear Filtering", motionblurGroup, true);
	page->addChild(*bilinearParam);
}

ImageEffect* GainPluginFactory::createInstance(OfxImageEffectHandle p_Handle, ContextEnum /*p_Context*/)
{
    return new GainPlugin(p_Handle);
}

void OFX::Plugin::getPluginIDs(PluginFactoryArray& p_FactoryArray)
{
    static GainPluginFactory gainPlugin;
    p_FactoryArray.push_back(&gainPlugin);
}
