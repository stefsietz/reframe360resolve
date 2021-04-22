#include "Reframe360.h"
#include "ofxsImageEffect.h"
#include "ofxsMultiThread.h"
#include "ofxsProcessing.h"

#define kPluginName "Reframe360 XL"
#define kPluginGrouping "Reframe360 XL"
#define kPluginDescription "Lets you animate a virtual camera in your 360 footage."
#define kPluginIdentifier "net.lerondpoint.Reframe360XLPlugin"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

#define kSupportsTiles false
#define kSupportsMultiResolution false
#define kSupportsMultipleClipPARs false

#define MAX_CAM_NUM 20

#define M_PI 3.14159265358979323846

///GLM Imports

#include "MathUtil.h"
#include <ctime>

////////////////////////////////////////////////////////////////////////////////

class ImageScaler : public OFX::ImageProcessor
{
public:
    explicit ImageScaler(OFX::ImageEffect& p_Instance);

#ifndef __APPLE__
    virtual void processImagesCUDA();
#endif

    virtual void processImagesOpenCL();

#if defined(__APPLE__)
    virtual void processImagesMetal();
#endif

    virtual void multiThreadProcessImages(OfxRectI p_ProcWindow);

    void setSrcImg(OFX::Image* p_SrcImg);
    void setParams(float* p_RotMat, float* p_Fov, float* p_Tinyplanet, float* p_Rectilinear, int p_Samples,
                   bool p_Bilinear);

private:
    OFX::Image* _srcImg;
    float* _rotMat;
    float* _fov;
    float* _tinyplanet;
    float* _rectilinear;
    int _samples;
    bool _bilinear;
};

ImageScaler::ImageScaler(OFX::ImageEffect& p_Instance)
    : OFX::ImageProcessor(p_Instance)
{
}

void pitchMatrix(float pitch, float** out)
{
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

void yawMatrix(float yaw, float** out)
{
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

void rollMatrix(float roll, float** out)
{
    (*out)[0] = cos(roll);
    (*out)[1] = -sin(roll);
    (*out)[2] = 0;
    (*out)[3] = sin(roll);
    (*out)[4] = cos(roll);
    (*out)[5] = 0;
    (*out)[6] = 0;
    (*out)[7] = 0;
    (*out)[8] = 1.0;
}


void matMul(const float* y, const float* p, float** outmat)
{
    (*outmat)[0] = p[0] * y[0] + p[3] * y[1] + p[6] * y[2];
    (*outmat)[1] = p[1] * y[0] + p[4] * y[1] + p[7] * y[2];
    (*outmat)[2] = p[2] * y[0] + p[5] * y[1] + p[8] * y[2];
    (*outmat)[3] = p[0] * y[3] + p[3] * y[4] + p[6] * y[5];
    (*outmat)[4] = p[1] * y[3] + p[4] * y[4] + p[7] * y[5];
    (*outmat)[5] = p[2] * y[3] + p[5] * y[4] + p[8] * y[5];
    (*outmat)[6] = p[0] * y[6] + p[3] * y[7] + p[6] * y[8];
    (*outmat)[7] = p[1] * y[6] + p[4] * y[7] + p[7] * y[8];
    (*outmat)[8] = p[2] * y[6] + p[5] * y[7] + p[8] * y[8];
}

// There is no CUDA on MacOS
#ifndef __APPLE__
extern void RunCudaKernel(int p_Width, int p_Height, float* p_Fov, float* p_Tinyplanet, float* p_Rectilinear,
                          const float* p_Input, float* p_Output, const float* p_RotMat, int p_Samples, bool p_Bilinear);

void ImageScaler::processImagesCUDA()
{
    const OfxRectI& bounds = _srcImg->getBounds();
    const int width = bounds.x2 - bounds.x1;
    const int height = bounds.y2 - bounds.y1;

    float* input = static_cast<float*>(_srcImg->getPixelData());
    float* output = static_cast<float*>(_dstImg->getPixelData());

    RunCudaKernel(width, height, _fov, _tinyplanet, _rectilinear, input, output, _rotMat, _samples, _bilinear);
}
#endif

//diasble Metal since there is not yet a metal kernel
#if defined(__APPLE__)
extern void RunMetalKernel(void* p_CmdQ, int p_Width, int p_Height, float* p_Fov, float* p_Tinyplanet, float* p_Rectilinear, const float* p_Input, float* p_Output,float* p_RotMat, int p_Samples,
                            bool p_Bilinear);

void ImageScaler::processImagesMetal()
{
    const OfxRectI& bounds = _srcImg->getBounds();
    const int width = bounds.x2 - bounds.x1;
    const int height = bounds.y2 - bounds.y1;

    float* input = static_cast<float*>(_srcImg->getPixelData());
    float* output = static_cast<float*>(_dstImg->getPixelData());

    RunMetalKernel(_pMetalCmdQ, width, height, _fov, _tinyplanet, _rectilinear, input, output, _rotMat, _samples, _bilinear);
}
#endif

//#if defined(__OPENCL__)
extern void RunOpenCLKernel(void* p_CmdQ, int p_Width, int p_Height, float* p_Fov, float* p_Tinyplanet,
                            float* p_Rectilinear, const float* p_Input, float* p_Output, float* p_RotMat, int p_Samples,
                            bool p_Bilinear);

void ImageScaler::processImagesOpenCL()
{
    const OfxRectI& bounds = _srcImg->getBounds();
    const int width = bounds.x2 - bounds.x1;
    const int height = bounds.y2 - bounds.y1;

    float* input = static_cast<float*>(_srcImg->getPixelData());
    float* output = static_cast<float*>(_dstImg->getPixelData());

    RunOpenCLKernel(_pOpenCLCmdQ, width, height, _fov, _tinyplanet, _rectilinear, input, output, _rotMat, _samples,
                    _bilinear);
}
//#endif


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
                vec2 uv = vec2((float)x / width, (float)y / height);

                vec3 dir = vec3(0, 0, 0);
                dir.x = (uv.x * 2) - 1;
                dir.y = (uv.y * 2) - 1;
                dir.y /= aspect;
                dir.z = fov;

                vec3 tinyplanet = tinyPlanetSph(dir);
                tinyplanet = normalize(tinyplanet);

                tinyplanet = rotMat * tinyplanet;
                vec3 rectdir = rotMat * dir;

                rectdir = normalize(rectdir);

                dir = mix(fisheyeDir(dir, rotMat), tinyplanet, _tinyplanet[i]);
                dir = mix(dir, rectdir, _rectilinear[i]);

                vec2 iuv = polarCoord(dir);
                iuv = repairUv(iuv);

                iuv.x *= (width - 1);
                iuv.y *= (height - 1);

                int x_new = p_ProcWindow.x1 + (int)iuv.x;
                int y_new = p_ProcWindow.y1 + (int)iuv.y;

                if ((x_new < width) && (y_new < height))
                {
                    float* srcPix = static_cast<float*>(_srcImg ? _srcImg->getPixelAddress(x_new, y_new) : 0);
                    vec4 interpCol;

                    // do we have a source image to scale up
                    if (srcPix)
                    {
                        if (_bilinear)
                        {
                            interpCol = linInterpCol(iuv, _srcImg, p_ProcWindow, width, height);
                        }
                        else
                        {
                            interpCol = vec4(srcPix[0], srcPix[1], srcPix[2], srcPix[3]);
                        }
                    }
                    else
                    {
                        interpCol = vec4(0, 0, 0, 1.0);
                    }

                    if (i == 0)
                    {
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

void ImageScaler::setParams(float* p_RotMat, float* p_Fov, float* p_Tinyplanet, float* p_Rectilinear, int p_Samples,
                            bool p_Bilinear)
{
    _rotMat = p_RotMat;
    _fov = p_Fov;
    _tinyplanet = p_Tinyplanet;
    _rectilinear = p_Rectilinear;
    _samples = p_Samples;
    _bilinear = p_Bilinear;
}

////////////////////////////////////////////////////////////////////////////////
/** @brief The plugin that does our work */
class Reframe360 : public OFX::ImageEffect
{
public:
    explicit Reframe360(OfxImageEffectHandle p_Handle);

    /* Override the render */
    virtual void render(const OFX::RenderArguments& p_Args);

    /* Override is identity */
    virtual bool isIdentity(const OFX::IsIdentityArguments& p_Args, OFX::Clip*& p_IdentityClip, double& p_IdentityTime);

    /* Override changedParam */
    virtual void changedParam(const OFX::InstanceChangedArgs& p_Args, const std::string& p_ParamName);

    /* Override changed clip */
    virtual void changedClip(const OFX::InstanceChangedArgs& p_Args, const std::string& p_ClipName);

    /* Set the enabledness of the component scale params depending on the type of input image and the state of the scaleComponents param */
    void setEnabledness() const;

    void setActiveParams();

    std::string paramIdForCam(std::string baseName, int cam);

    void setHiddenParam(std::string name, int cam, double value);

    template<class TParam>
    float BlendParam(TParam* param, const OFX::RenderArguments& p_Args, float offset) const;

    float ApplyBlendCurve(float blend, const OFX::RenderArguments& p_Args, float offset) const;
    float BlendFromAlpha(float alpha, const OFX::RenderArguments& p_Args, float offset) const;

    /* Set up and run a processor */
    void setupAndProcess(ImageScaler& p_ImageScaler, const OFX::RenderArguments& p_Args);

private:
    // Does not own the following pointers
    OFX::Clip* m_DstClip;
    OFX::Clip* m_SrcClip;

    OFX::DoubleParam* m_Pitch;
    OFX::DoubleParam* m_Yaw;
    OFX::DoubleParam* m_Roll;
    OFX::DoubleParam* m_Fov;

    OFX::IntParam* m_ActiveCamera;
    OFX::BooleanParam* m_ShowActiveCamera;
    OFX::PushButtonParam* m_CopyButton;
    OFX::PushButtonParam* m_PasteButton;
    OFX::IntParam* m_CopyValue;

    OFX::IntParam* m_CameraSequence;
    OFX::ChoiceParam* m_BlendCurve;
    OFX::DoubleParam* m_Accel;

    OFX::DoubleParam* m_Pitch1;
    OFX::DoubleParam* m_Yaw1;
    OFX::DoubleParam* m_Roll1;
    OFX::DoubleParam* m_Fov1;
    OFX::DoubleParam* m_Tinyplanet1;
    OFX::DoubleParam* m_Recti1;

    OFX::DoubleParam* m_Pitch2[MAX_CAM_NUM];
    OFX::DoubleParam* m_Yaw2[MAX_CAM_NUM];
    OFX::DoubleParam* m_Roll2[MAX_CAM_NUM];
    OFX::DoubleParam* m_Fov2[MAX_CAM_NUM];
    OFX::DoubleParam* m_Tinyplanet2[MAX_CAM_NUM];
    OFX::DoubleParam* m_Recti2[MAX_CAM_NUM];

    OFX::DoubleParam* m_Shutter;
    OFX::IntParam* m_Samples;
    OFX::BooleanParam* m_Bilinear;

    template <class TParam>
    static double getNextKeyframeTime(TParam* param, double time);
    template <class TParam>
    static double getPreviousKeyframeTime(TParam* param, double time);

    template <class TParam>
    static float getPreviousKeyframeValue(TParam* param, double time);

    template <class TParam>
    static float getNextKeyframeValue(TParam* param, double time);

    template <class TParam>
    static float getRelativeKeyframeAlpha(TParam* param, double time, double offset);

    template<class TParam>
    static bool needsInterPolation(TParam* param, double time);

    template<class TParam>
    static bool isFirstKeyFrameTimeOrEarlier(TParam* param, double time);

    template<class TParam>
    static bool isLastKeyFrameTimeOrLater(TParam* param, double time);

    template<class TParam>
    static bool isExactlyOnKeyFrame(TParam* param, double time);

    //ReframeParamSet m_ParamStruct;
};

Reframe360::Reframe360(OfxImageEffectHandle p_Handle)
    : ImageEffect(p_Handle)
{
    m_DstClip = fetchClip(kOfxImageEffectOutputClipName);
    m_SrcClip = fetchClip(kOfxImageEffectSimpleSourceClipName);

    m_Pitch = fetchDoubleParam("main_pitch");
    m_Yaw = fetchDoubleParam("main_yaw");
    m_Roll = fetchDoubleParam("main_roll");
    m_Fov = fetchDoubleParam("main_fov");

    m_CameraSequence = fetchIntParam("cam1");
    m_BlendCurve = fetchChoiceParam("blend_curve");
    m_Accel = fetchDoubleParam("accel");

    m_ActiveCamera = fetchIntParam("active_cam");
    m_ShowActiveCamera = fetchBooleanParam("show_active_cam");
    m_CopyButton = fetchPushButtonParam("copy_button");
    m_PasteButton = fetchPushButtonParam("paste_button");
    m_CopyValue = fetchIntParam("copy_value");

    m_Shutter = fetchDoubleParam("shutter");
    m_Samples = fetchIntParam("samples");
    m_Bilinear = fetchBooleanParam("bilinear");

    m_Pitch1 = fetchDoubleParam("aux_pitch");
    m_Yaw1 = fetchDoubleParam("aux_yaw");
    m_Roll1 = fetchDoubleParam("aux_roll");
    m_Fov1 = fetchDoubleParam("aux_fov");
    m_Tinyplanet1 = fetchDoubleParam("aux_tiny");
    m_Recti1 = fetchDoubleParam("aux_recti");

    for (int i = 0; i < MAX_CAM_NUM; i++)
    {
        m_Pitch2[i] = fetchDoubleParam(paramIdForCam("aux_pitch", i));
        m_Yaw2[i] = fetchDoubleParam(paramIdForCam("aux_yaw", i));
        m_Roll2[i] = fetchDoubleParam(paramIdForCam("aux_roll", i));
        m_Fov2[i] = fetchDoubleParam(paramIdForCam("aux_fov", i));
        m_Tinyplanet2[i] = fetchDoubleParam(paramIdForCam("aux_tiny", i));
        m_Recti2[i] = fetchDoubleParam(paramIdForCam("aux_recti", i));
    }

    // Set the enabledness of our RGBA sliders
    setEnabledness();
}

void Reframe360::render(const OFX::RenderArguments& p_Args)
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

bool Reframe360::isIdentity(const OFX::IsIdentityArguments& p_Args, OFX::Clip*& p_IdentityClip, double& p_IdentityTime)
{
    return false;
}

void Reframe360::changedParam(const OFX::InstanceChangedArgs& p_Args, const std::string& p_ParamName)
{
    if (p_ParamName.find("active_cam") == 0)
    {
        int activeCam = m_ActiveCamera->getValue();
        activeCam--;

        m_Pitch1->setValue(m_Pitch2[activeCam]->getValue());
        m_Yaw1->setValue(m_Yaw2[activeCam]->getValue());
        m_Roll1->setValue(m_Roll2[activeCam]->getValue());
        m_Fov1->setValue(m_Fov2[activeCam]->getValue());
        m_Tinyplanet1->setValue(m_Tinyplanet2[activeCam]->getValue());
        m_Recti1->setValue(m_Recti2[activeCam]->getValue());
    }
    else if (p_ParamName.find("hidden") == std::string::npos && p_ParamName.find("aux") == 0)
    {
        double value = fetchDoubleParam(p_ParamName)->getValue();
        int activeCam = fetchIntParam("active_cam")->getValue();

        std::stringstream ss;
        ss << p_ParamName << "_hidden_" << activeCam;
        std::string name = ss.str();

        setHiddenParam(name, activeCam, value);
    }
    else if (p_ParamName.find("copy_button") == 0)
    {
        m_CopyValue->setValue(m_ActiveCamera->getValue());
    }
    else if (p_ParamName.find("paste_button") == 0)
    {
        int activeCam = m_ActiveCamera->getValue();
        activeCam--;
        int copyCam = m_CopyValue->getValue();
        copyCam--;
        m_Pitch1->setValue(m_Pitch2[copyCam]->getValue());
        m_Yaw1->setValue(m_Yaw2[copyCam]->getValue());
        m_Roll1->setValue(m_Roll2[copyCam]->getValue());
        m_Fov1->setValue(m_Fov2[copyCam]->getValue());
        m_Tinyplanet1->setValue(m_Tinyplanet2[copyCam]->getValue());
        m_Recti1->setValue(m_Recti2[copyCam]->getValue());
    }
}

void Reframe360::changedClip(const OFX::InstanceChangedArgs& p_Args, const std::string& p_ClipName)
{
    if (p_ClipName == kOfxImageEffectSimpleSourceClipName)
    {
        setEnabledness();
    }
}

void Reframe360::setEnabledness() const
{
    // the component enabledness depends on the clip being RGBA and the param being true
    const bool enable = ((m_SrcClip->getPixelComponents() == OFX::ePixelComponentRGBA));

    /*
    m_Pitch->setEnabled(enable);
    m_Yaw->setEnabled(enable);
    m_Fov->setEnabled(enable);
    m_Recti->setEnabled(enable);

    m_Blend->setEnabled(enable);
    m_BlendCurve->setEnabled(enable);
    m_Accel->setEnabled(enable);

    m_Pitch2->setEnabled(enable);
    m_Yaw2->setEnabled(enable);
    m_Fov2->setEnabled(enable);
    m_Fisheye2->setEnabled(enable);
    */
}

static float interpParam(OFX::DoubleParam* param, const OFX::RenderArguments& p_Args, float offset)
{
    if (offset == 0)
    {
        return (float)param->getValueAtTime(p_Args.time);
    }
    else if (offset < 0)
    {
        offset = -offset;
        float floor = std::floor(offset);
        float frac = offset - floor;

        return (float)param->getValueAtTime(p_Args.time - (floor + 1)) * frac + (float)param->
            getValueAtTime(p_Args.time - floor) * (1 - frac);
    }
    else
    {
        float floor = std::floor(offset);
        float frac = offset - floor;

        return (float)param->getValueAtTime(p_Args.time + (floor + 1)) * frac + (float)param->
            getValueAtTime(p_Args.time + floor) * (1 - frac);
    }
}

static int interpParam(OFX::ChoiceParam* param, const OFX::RenderArguments& p_Args, float offset)
{
    float floor = std::floor(offset);
    int value;

    param->getValueAtTime(p_Args.time + floor, value);

    return value;
}

void Reframe360::setupAndProcess(ImageScaler& p_ImageScaler, const OFX::RenderArguments& p_Args)
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

    float pitch = 0.0f, roll = 0.0f, yaw = 0.0f, fov = 1.0f,
          pitch1 = 1.0f, yaw1 = 1.0f, roll1 = 0.0f, fov1 = 1.0f, tinyplanet1 = 1.0f, recti1 = 0.0f,
          pitch2 = 1.0f, yaw2 = 1.0f, roll2 = 0.0f, fov2 = 1.0f, tinyplanet2 = 1.0f, recti2 = 0.0f,
          blend = 0.0f;

    int mb_samples = (int)m_Samples->getValueAtTime(p_Args.time);
    float mb_shutter = (float)m_Shutter->getValueAtTime(p_Args.time) * 0.5f;
    int bilinear = m_Bilinear->getValueAtTime(p_Args.time);

    int activeCam = m_ActiveCamera->getValueAtTime(p_Args.time) - 1;

    int cam1 = static_cast<int>(getPreviousKeyframeValue(m_CameraSequence, p_Args.time)) - 1;
    int cam2 = static_cast<int>(getNextKeyframeValue(m_CameraSequence, p_Args.time)) - 1;

    bool showActiveCam = m_ShowActiveCamera->getValueAtTime(p_Args.time);

    if (showActiveCam)
    {
        cam1 = activeCam;
    }

    float* fovs = (float*)malloc(sizeof(float) * mb_samples);
    float* tinyplanets = (float*)malloc(sizeof(float) * mb_samples);
    float* rectilinears = (float*)malloc(sizeof(float) * mb_samples);
    float* rotmats = (float*)malloc(sizeof(float) * mb_samples * 9);

    for (int i = 0; i < mb_samples; i++)
    {
        float offset = 0;
        if (mb_samples > 1)
        {
            offset = fitRange((float)i, 0, mb_samples - 1.0f, -1.0f, 1.0f);
        }

        offset *= mb_shutter;

        pitch = BlendParam(m_Pitch, p_Args, offset);
        yaw = BlendParam(m_Yaw, p_Args, offset);
        roll = BlendParam(m_Roll, p_Args, offset);
        fov = BlendParam(m_Fov, p_Args, offset);

        pitch1 = interpParam(m_Pitch2[cam1], p_Args, offset);
        yaw1 = interpParam(m_Yaw2[cam1], p_Args, offset);
        roll1 = interpParam(m_Roll2[cam1], p_Args, offset);
        fov1 = interpParam(m_Fov2[cam1], p_Args, offset);
        tinyplanet1 = interpParam(m_Tinyplanet2[cam1], p_Args, offset);
        recti1 = interpParam(m_Recti2[cam1], p_Args, offset);

        pitch2 = interpParam(m_Pitch2[cam2], p_Args, offset);
        yaw2 = interpParam(m_Yaw2[cam2], p_Args, offset);
        roll2 = interpParam(m_Roll2[cam2], p_Args, offset);
        fov2 = interpParam(m_Fov2[cam2], p_Args, offset);
        tinyplanet2 = interpParam(m_Tinyplanet2[cam2], p_Args, offset);
        recti2 = interpParam(m_Recti2[cam2], p_Args, offset);

        float camBlend = 0;
        if (cam1 != cam2)
            camBlend = getRelativeKeyframeAlpha(m_CameraSequence, p_Args.time, offset);

        if (showActiveCam)
        {
            blend = 0;
        }
        else
        {
            blend = BlendFromAlpha(camBlend, p_Args, offset);
        }

        pitch = pitch1 + (pitch2 - pitch1) * blend + pitch;
        yaw = yaw1 + (yaw2 - yaw1) * blend + yaw;
        roll = roll1 + (roll2 - roll1) * blend + roll;
        fov = (fov1 + (fov2 - fov1) * blend) * fov;
        tinyplanet1 = tinyplanet1 + (tinyplanet2 - tinyplanet1) * blend;
        recti1 = recti1 + (recti2 - recti1) * blend;

        float* pitchMat = (float*)calloc(9, sizeof(float));
        pitchMatrix(-pitch / 180 * (float)M_PI, &pitchMat);
        float* yawMat = (float*)calloc(9, sizeof(float));
        yawMatrix(yaw / 180 * (float)M_PI, &yawMat);
        float* rollMat = (float*)calloc(9, sizeof(float));
        rollMatrix(-roll / 180 * (float)M_PI, &rollMat);

        float* pitchYawMat = (float*)calloc(9, sizeof(float));
        float* rotMat = (float*)calloc(9, sizeof(float));
        matMul(pitchMat, rollMat, &pitchYawMat);
        matMul(yawMat, pitchYawMat, &rotMat);

        free(pitchMat);
        free(yawMat);
        free(rollMat);

        memcpy(&(rotmats[i * 9]), rotMat, sizeof(float) * 9);
        free(rotMat);

        fovs[i] = static_cast<float>(fov);
        tinyplanets[i] = static_cast<float>(tinyplanet1);
        rectilinears[i] = static_cast<float>(recti1);
    }

    // Set the images
    p_ImageScaler.setDstImg(dst.get());
    p_ImageScaler.setSrcImg(src.get());

    // Setup OpenCL and CUDA Render arguments
    p_ImageScaler.setGPURenderArgs(p_Args);

    // Set the render window
    p_ImageScaler.setRenderWindow(p_Args.renderWindow);

    // Set the scales
    p_ImageScaler.setParams(rotmats, fovs, tinyplanets, rectilinears, mb_samples, bilinear);

    // Call the base class process member, this will call the derived templated process code
    p_ImageScaler.process();
}

////////////////////////////////////////////////////////////////////////////////

using namespace OFX;

Reframe360Factory::Reframe360Factory()
    : OFX::PluginFactoryHelper<Reframe360Factory>(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor)
{
}

void Reframe360Factory::describe(OFX::ImageEffectDescriptor& p_Desc)
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
    #ifndef __APPLE__
    p_Desc.setSupportsCudaRender(true);
    #endif
    #ifdef __APPLE__
    p_Desc.setSupportsMetalRender(true);
    #endif
}

static DoubleParamDescriptor* defineParam(OFX::ImageEffectDescriptor& p_Desc, const std::string& p_Name,
                                          const std::string& p_Label,
                                          const std::string& p_Hint, GroupParamDescriptor* p_Parent, float min,
                                          float max, float default_value, float hardmin = INT_MIN, float hardmax = INT_MAX)
{
    DoubleParamDescriptor* param = p_Desc.defineDoubleParam(p_Name);

    param->setLabels(p_Label, p_Label, p_Label);
    param->setScriptName(p_Name);
    param->setHint(p_Hint);
    param->setDefault(default_value);
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

static DoubleParamDescriptor* defineAngleParam(OFX::ImageEffectDescriptor& p_Desc, const std::string& p_Name,
                                               const std::string& p_Label,
                                               const std::string& p_Hint, GroupParamDescriptor* p_Parent, float min,
                                               float max, float default_value, float hardmin = INT_MIN,
                                               float hardmax = INT_MAX)
{
    DoubleParamDescriptor* param = p_Desc.defineDoubleParam(p_Name);

    param->setLabels(p_Label, p_Label, p_Label);
    param->setScriptName(p_Name);
    param->setHint(p_Hint);
    param->setDefault(default_value);
    param->setRange(hardmin, hardmax);
    param->setIncrement(0.1);
    param->setDisplayRange(min, max);
    param->setDoubleType(eDoubleTypeAngle);

    if (p_Parent)
    {
        param->setParent(*p_Parent);
    }

    return param;
}

static IntParamDescriptor* defineIntParam(OFX::ImageEffectDescriptor& p_Desc, const std::string& p_Name,
                                          const std::string& p_Label,
                                          const std::string& p_Hint, GroupParamDescriptor* p_Parent, int min, int max,
                                          int default_value, int hardmin = INT_MIN, int hardmax = INT_MAX)
{
    IntParamDescriptor* param = p_Desc.defineIntParam(p_Name);

    param->setLabels(p_Label, p_Label, p_Label);
    param->setScriptName(p_Name);
    param->setHint(p_Hint);
    param->setDefault(default_value);
    param->setRange(hardmin, hardmax);
    param->setDisplayRange(min, max);

    if (p_Parent)
    {
        param->setParent(*p_Parent);
    }

    return param;
}

static ChoiceParamDescriptor* defineChoiceParam(OFX::ImageEffectDescriptor& p_Desc, const std::string& p_Name,
                                                const std::string& p_Label,
                                                const std::string& p_Hint, GroupParamDescriptor* p_Parent, int default_value,
                                                std::string p_ChoiceLabels[], int choiceCount)
{
    ChoiceParamDescriptor* param = p_Desc.defineChoiceParam(p_Name);

    param->setLabels(p_Label, p_Label, p_Label);
    param->setScriptName(p_Name);
    param->setHint(p_Hint);
    param->setDefault(default_value);

    for (int i = 0; i < choiceCount; ++i)
    {
        auto choice = p_ChoiceLabels[i];

        param->appendOption(choice, choice);
    }

    if (p_Parent)
    {
        param->setParent(*p_Parent);
    }

    return param;
}

static BooleanParamDescriptor* defineBooleanParam(OFX::ImageEffectDescriptor& p_Desc, const std::string& p_Name,
                                                  const std::string& p_Label,
                                                  const std::string& p_Hint, GroupParamDescriptor* p_Parent,
                                                  bool default_value)
{
    BooleanParamDescriptor* param = p_Desc.defineBooleanParam(p_Name);

    param->setLabels(p_Label, p_Label, p_Label);
    param->setScriptName(p_Name);
    param->setHint(p_Hint);
    param->setDefault(default_value);

    if (p_Parent)
    {
        param->setParent(*p_Parent);
    }

    return param;
}

static PushButtonParamDescriptor* defineButtonParam(OFX::ImageEffectDescriptor& p_Desc, const std::string& p_Name,
                                                    const std::string& p_Label,
                                                    const std::string& p_Hint, GroupParamDescriptor* p_Parent)
{
    PushButtonParamDescriptor* param = p_Desc.definePushButtonParam(p_Name);

    param->setLabels(p_Label, p_Label, p_Label);
    param->setScriptName(p_Name);
    param->setHint(p_Hint);

    if (p_Parent)
    {
        param->setParent(*p_Parent);
    }

    return param;
}


void Reframe360Factory::describeInContext(OFX::ImageEffectDescriptor& p_Desc, OFX::ContextEnum /*p_Context*/)
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

    GroupParamDescriptor* camera1ParamsGroup = p_Desc.defineGroupParam("mainCameraParams");
    camera1ParamsGroup->setHint("Main Camera Parameters");
    camera1ParamsGroup->setLabels("Main Camera Parameters", "Main Camera Parameters", "Main Camera Parameters");


    // Make the camera 1 params
    DoubleParamDescriptor* param = defineAngleParam(p_Desc, "main_pitch", "Pitch", "Up/down camera rotation.  Displayed value might not be in sync with actual camera depending on animation curve parameters.",
                                                    camera1ParamsGroup, -90, 90, 0);
    page->addChild(*param);

    param = defineAngleParam(p_Desc, "main_yaw", "Yaw", "Left/right camera rotation.  Displayed value might not be in sync with actual camera depending on animation curve parameters.", camera1ParamsGroup, -180, 180, 0);
    page->addChild(*param);

    param = defineAngleParam(p_Desc, "main_roll", "Roll", "Camera Roll.  Displayed value might not be in sync with actual camera depending on animation curve parameters.", camera1ParamsGroup, -180, 180, 0);
    page->addChild(*param);

    param = defineParam(p_Desc, "main_fov", "Field of View", "Camera field of view.  Displayed value might not be in sync with actual camera depending on animation curve parameters.", camera1ParamsGroup, 0.15f, 5, 1);
    page->addChild(*param);

    GroupParamDescriptor* animGroup = p_Desc.defineGroupParam("animParams");
    animGroup->setHint("Camera Animation Parameters");
    animGroup->setLabels("Camera Animation Parameters", "Camera Animation Parameters", "Camera Animation Parameters");

    IntParamDescriptor* intParam = defineIntParam(p_Desc, "cam1", "Camera Sequence", "Camera Sequence", animGroup, 1,
                                                  20, 1);
    page->addChild(*intParam);

    std::string choices[] = {"Power", "Sine", "Exponential", "Circular"};
    ChoiceParamDescriptor* choiceParam = defineChoiceParam(p_Desc, "blend_curve", "Blend Curve", "Blend Curve",
                                                           animGroup, 0, choices, 4);
    page->addChild(*choiceParam);

    param = defineParam(p_Desc, "accel", "Power Acceleration", "Power Acceleration (only used for Power curve mode)", animGroup, 1, 20, 3, 0.5, 20);
    page->addChild(*param);

    GroupParamDescriptor* cameraSelectionParamsGroup = p_Desc.defineGroupParam("cameraSelectionParams");
    cameraSelectionParamsGroup->setHint("Camera Selection Parameters");
    cameraSelectionParamsGroup->setLabels("Camera Selection Parameters", "Camera Selection Parameters",
                                         "Camera Selection Parameters");

    intParam = defineIntParam(p_Desc, "active_cam", "Edit Camera", "Edit Camera", cameraSelectionParamsGroup, 1,
                              MAX_CAM_NUM, 1, 1, 20);
    page->addChild(*intParam);

    BooleanParamDescriptor* boolParam = defineBooleanParam(p_Desc, "show_active_cam", "Show Edit Camera",
                                                           "Show Edit Camera", cameraSelectionParamsGroup, false);
    page->addChild(*boolParam);

    GroupParamDescriptor* camera2ParamsGroup = p_Desc.defineGroupParam("auxCameraParams");
    camera2ParamsGroup->setHint("Aux Camera Parameters");
    camera2ParamsGroup->setLabels("Aux Camera Parameters", "Aux Camera Parameters", "Aux Camera Parameters");

    PushButtonParamDescriptor* buttonParam = defineButtonParam(p_Desc, "copy_button", "Copy Camera", "Copy Camera",
                                                               camera2ParamsGroup);
    page->addChild(*buttonParam);

    buttonParam = defineButtonParam(p_Desc, "paste_button", "Paste Camera", "Paste Camera", camera2ParamsGroup);
    page->addChild(*buttonParam);

    intParam = defineIntParam(p_Desc, "copy_value", "", "", camera2ParamsGroup, 1, 20, 1);
    intParam->setIsSecret(true);
    page->addChild(*intParam);

    // Make the camera 2 params
    param = defineAngleParam(p_Desc, "aux_pitch", "Pitch", "Up/down camera rotation", camera2ParamsGroup, -90, 90, 0);
    page->addChild(*param);

    param = defineAngleParam(p_Desc, "aux_yaw", "Yaw", "Left/right camera rotation", camera2ParamsGroup, -180, 180, 0);
    page->addChild(*param);

    param = defineAngleParam(p_Desc, "aux_roll", "Roll", "Camera Roll", camera2ParamsGroup, -180, 180, 0);
    page->addChild(*param);

    param = defineParam(p_Desc, "aux_fov", "Field of View", "Camera field of view", camera2ParamsGroup, 0.15f, 5, 1);
    page->addChild(*param);

    param = defineParam(p_Desc, "aux_recti", "Rectilinear Projection", "Rectilinear Projection", camera2ParamsGroup, 0,
                        1, 0, 0, 1);
    page->addChild(*param);

    param = defineParam(p_Desc, "aux_tiny", "Tiny Planet",
                        "Blend between standard Fisheye projection and Tiny Planet Projection", camera2ParamsGroup, 0,
                        1, 0, 0, 1);
    page->addChild(*param);

    //hidden params
    GroupParamDescriptor* hiddenCameraParamsGroup = p_Desc.defineGroupParam("hiddenCameraParams");
    hiddenCameraParamsGroup->setHint("Camera Parameters");
    hiddenCameraParamsGroup->setLabels("Camera Parameters", "Camera Parameters", "Camera Parameters");
    hiddenCameraParamsGroup->setIsSecret(true);

    for (int i = 0; i < MAX_CAM_NUM; i++)
    {
        param = defineAngleParam(p_Desc, paramIdForCam("aux_pitch", i), "Pitch", "Up/down camera rotation",
                                 hiddenCameraParamsGroup, -90, 90, 0);
        page->addChild(*param);

        param = defineAngleParam(p_Desc, paramIdForCam("aux_yaw", i), "Yaw", "Left/right camera rotation",
                                 hiddenCameraParamsGroup, -180, 180, 0);
        page->addChild(*param);

        param = defineAngleParam(p_Desc, paramIdForCam("aux_roll", i), "Roll", "Camera Roll", hiddenCameraParamsGroup,
                                 -180, 180, 0);
        page->addChild(*param);

        param = defineParam(p_Desc, paramIdForCam("aux_fov", i), "Field of View", "Camera field of view",
                            hiddenCameraParamsGroup, 0.15f, 5, 1);
        page->addChild(*param);

        param = defineParam(p_Desc, paramIdForCam("aux_tiny", i), "Tiny Planet",
                            "Blend between standard Fisheye projection and Tiny Planet Projection",
                            hiddenCameraParamsGroup, 0, 1, 0, 0, 1);
        page->addChild(*param);

        param = defineParam(p_Desc, paramIdForCam("aux_recti", i), "Rectify Projection", "Rectify Projection",
                            hiddenCameraParamsGroup, 0, 1, 0, 0, 1);
        page->addChild(*param);
    }

    GroupParamDescriptor* motionblurGroup = p_Desc.defineGroupParam("motionblurParams");
    motionblurGroup->setHint("Motion Camera Parameters");
    motionblurGroup->setLabels("Motion Blur Parameters", "Motion Blur Parameters", "Motion Blur Parameters");

    param = defineParam(p_Desc, "shutter", "Shutter Angle", "Shutter Angle", motionblurGroup, 0, 1, 0, 0, 3);
    page->addChild(*param);

    intParam = defineIntParam(p_Desc, "samples", "Samples", "Samples", motionblurGroup, 1, 20, 1, 1, 256);
    page->addChild(*intParam);

    BooleanParamDescriptor* bilinearParam = defineBooleanParam(p_Desc, "bilinear", "Bilinear Filtering",
                                                               "Bilinear Filtering", motionblurGroup, true);
    page->addChild(*bilinearParam);
}

ImageEffect* Reframe360Factory::createInstance(OfxImageEffectHandle p_Handle, ContextEnum /*p_Context*/)
{
    return new Reframe360(p_Handle);
}

void OFX::Plugin::getPluginIDs(PluginFactoryArray& p_FactoryArray)
{
    static Reframe360Factory reframe360;
    p_FactoryArray.push_back(&reframe360);
}


std::string Reframe360Factory::paramIdForCam(std::string baseName, int cam)
{
    std::stringstream ss;
    ss << baseName << "_hidden_" << cam;
    return ss.str();
}

std::string Reframe360::paramIdForCam(std::string baseName, int cam)
{
    std::stringstream ss;
    ss << baseName << "_hidden_" << cam;
    return ss.str();
}

void Reframe360::setHiddenParam(std::string name, int cam, double value)
{
    cam--;

    if (name.find("pitch") != std::string::npos)
    {
        m_Pitch2[cam]->setValue(value);
    }
    else if (name.find("yaw") != std::string::npos)
    {
        m_Yaw2[cam]->setValue(value);
    }
    else if (name.find("roll") != std::string::npos)
    {
        m_Roll2[cam]->setValue(value);
    }
    else if (name.find("fov") != std::string::npos)
    {
        m_Fov2[cam]->setValue(value);
    }
    else if (name.find("tiny") != std::string::npos)
    {
        m_Tinyplanet2[cam]->setValue(value);
    }
    else if (name.find("recti") != std::string::npos)
    {
        m_Recti2[cam]->setValue(value);
    }
}

template<class TParam>
float Reframe360::BlendParam(TParam* param, const OFX::RenderArguments& p_Args, float offset) const
{
    auto alpha = getRelativeKeyframeAlpha(param, p_Args.time, offset);
    auto blend = BlendFromAlpha(alpha, p_Args, offset);
    auto previousKeyframeValue = getPreviousKeyframeValue(param, p_Args.time);
    auto nextKeyframeValue = getNextKeyframeValue(param, p_Args.time);
    auto blendedValue = previousKeyframeValue + (nextKeyframeValue - previousKeyframeValue) * blend;

    return blendedValue;
}

float Reframe360::BlendFromAlpha(float alpha, const OFX::RenderArguments& p_Args, float offset) const
{
    float blend = alpha;
    float range[4];

    if (blend < 0.5)
    {
        range[0] = 0;
        range[1] = 0.5;
        range[2] = 0;
        range[3] = 1;
    }
    else
    {
        range[0] = 0.5;
        range[1] = 1;
        range[2] = 1;
        range[3] = 0;
    }

    blend = fitRange(blend, range[0], range[1], range[2], range[3]);
    blend = ApplyBlendCurve(blend, p_Args, offset);
    blend = fitRange(blend, range[2], range[3], range[0], range[1]);

    return blend;
}

float Reframe360::ApplyBlendCurve(float alpha, const OFX::RenderArguments& p_Args, float offset) const
{
    int blendCurve = interpParam(m_BlendCurve, p_Args, offset);
    float blend;

    switch (blendCurve)
    {
    case 0: // Power
    {
        float accel = interpParam(m_Accel, p_Args, offset);

        blend = std::pow(alpha, accel);
        break;
    }
    case 1: // Sine
    {
        blend = 1 - static_cast<float>(std::cos((alpha * M_PI) / 2));
        break;
    }
    case 2: // Exponential
    {
        blend = alpha == 0 ? 0 : static_cast<float>(std::pow(2, 10 * alpha - 10));
        break;
    }
    case 3: // Circular
    {
        blend = 1 - static_cast<float>(sqrt(1 - pow(alpha, 2)));
        break;
    }
    default: // Linear
    {
        blend = alpha;
        break;
    }
    }

    return blend;
}

template<class TParam>
bool Reframe360::needsInterPolation(TParam* param, double time)
{
    if (isFirstKeyFrameTimeOrEarlier(param, time))
    {
        return false;
    }
    else if (isLastKeyFrameTimeOrLater(param, time))
    {
        return false;
    }
    else if (isExactlyOnKeyFrame(param, time))
    {
        return false;
    }
    else
    {
        return true;
    }
}

template<class TParam>
bool Reframe360::isExactlyOnKeyFrame(TParam* param, double time)
{
    double keyTime = 0;
    int keyIndex = param->getKeyIndex(time, eKeySearchNear);
    if (keyIndex == -1)
    {
        return false;
    }
    else
    {
        keyTime = param->getKeyTime(keyIndex);
        if (keyTime == time)
            return true;
        else
            return false;
    }
}

template<class TParam>
bool Reframe360::isFirstKeyFrameTimeOrEarlier(TParam* param, double time)
{
    int keyIndex = param->getKeyIndex(time, eKeySearchBackwards);
    if (keyIndex == -1)
        return true;
    else
        return false;
}

template<class TParam>
bool Reframe360::isLastKeyFrameTimeOrLater(TParam* param, double time)
{
    int keyIndex = param->getKeyIndex(time, eKeySearchForwards);
    if (keyIndex == -1)
        return true;
    else
        return false;
}

template <class TParam>
double Reframe360::getPreviousKeyframeTime(TParam* param, double time)
{
    int keyIndex = param->getKeyIndex(time, eKeySearchBackwards);
    return param->getKeyTime(keyIndex);
}

template <class TParam>
double Reframe360::getNextKeyframeTime(TParam* param, double time)
{
    int keyIndex = param->getKeyIndex(time, eKeySearchForwards);
    return param->getKeyTime(keyIndex);
}

template <class TParam>
float Reframe360::getRelativeKeyframeAlpha(TParam* param, double time, double offset)
{
    double currentTime = time;
    double offsetTime = currentTime + offset;
    const double prevTime = getPreviousKeyframeTime(param, currentTime);
    const double nextTime = getNextKeyframeTime(param, currentTime);
    const double prevTimeOffset = getPreviousKeyframeTime(param, offsetTime);
    const double nextTimeOffset = getNextKeyframeTime(param, offsetTime);

    if (prevTime == prevTimeOffset && nextTime == nextTimeOffset)
    {
        currentTime = offsetTime;
    }

    const double totalDiff = nextTime - prevTime;
    const double prevDiff = currentTime - prevTime;

    return totalDiff > 0 ? static_cast<float>(prevDiff / totalDiff) : 0;
}

template <class TParam>
float Reframe360::getPreviousKeyframeValue(TParam* param, double time)
{
    float outValue = 0;
    double queryTime = time;

    if (needsInterPolation(param, time))
    {
        queryTime = getPreviousKeyframeTime(param, time);
    }

    outValue = static_cast<float>(param->getValueAtTime(queryTime));

    return outValue;
}

template <class TParam>
float Reframe360::getNextKeyframeValue(TParam* param, double time)
{
    float outValue = 0;
    double queryTime = time;

    if (needsInterPolation(param, time))
    {
        queryTime = getNextKeyframeTime(param, time);
    }

    outValue = static_cast<float>(param->getValueAtTime(queryTime));

    return outValue;
}
