#include "distortionPass.h"

void DistortionPass::Init()
{
    m_Capture.Init();
}

void DistortionPass::Uninit()
{
    m_Capture.Uninit();
}

void DistortionPass::Capture()
{
    m_Capture.Capture();
}
