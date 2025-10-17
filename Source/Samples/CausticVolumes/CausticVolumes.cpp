/***************************************************************************
 # Copyright (c) 2020, NVIDIA CORPORATION. All rights reserved.
 #
 # Redistribution and use in source and binary forms, with or without
 # modification, are permitted provided that the following conditions
 # are met:
 #  * Redistributions of source code must retain the above copyright
 #    notice, this list of conditions and the following disclaimer.
 #  * Redistributions in binary form must reproduce the above copyright
 #    notice, this list of conditions and the following disclaimer in the
 #    documentation and/or other materials provided with the distribution.
 #  * Neither the name of NVIDIA CORPORATION nor the names of its
 #    contributors may be used to endorse or promote products derived
 #    from this software without specific prior written permission.
 #
 # THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS "AS IS" AND ANY
 # EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 # IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 # PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 # CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 # EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 # PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 # PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 # OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 # (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 # OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **************************************************************************/
#include "CausticVolumes.h"
//#include <Core/Sample.cpp>
uint32_t mSampleGuiWidth = 250;
uint32_t mSampleGuiHeight = 200;
uint32_t mSampleGuiPositionX = 20;
uint32_t mSampleGuiPositionY = 40;

namespace {
    const char kMarkerShaderFile[] = "Samples/CausticVolumes/Visualization2d.ps.slang";
    const char kNormalsShaderFile[] = "Samples/CausticVolumes/VoxelNormals.ps.slang";

    static const float4 kClearColor(0.38f, 0.52f, 0.10f, 1);
    static const std::string kDefaultScene = "Arcade/Arcade.pyscene";

    const Gui::DropdownList kModeList = {
        {(uint32_t)CausticVolumes::SceneOptions::MarkerDemo, "Marker demo"},
        {(uint32_t)CausticVolumes::SceneOptions::VoxelNormals, "Voxel normals"},
    };
}
void CausticVolumes::onGuiRender(Gui* pGui)
{
    /*Gui::Window w(pGui, "Falcor", { 250, 200 });
    gpFramework->renderGlobalUI(pGui);
    w.text("Hello from CausticVolumes");
    if (w.button("Click Here"))
    {
        msgBox("Now why would you do that?");
    }*/
    Gui::Window w(pGui, "Visualization 2D", { 700, 900 }, { 10, 10 });
    bool changed = w.dropdown("Scene selection", kModeList, reinterpret_cast<uint32_t&>(mSelectedScene));
    if (changed)
    {
        createRenderPass();
    }
    bool paused = gpFramework->getGlobalClock().isPaused();
    changed = w.checkbox("Pause time", paused);
    if (changed)
    {
        if (paused)
        {
            gpFramework->getGlobalClock().pause();
        }
        else
        {
            gpFramework->getGlobalClock().play();
        }
    }

    //renderGlobalUI(pGui);
    if (mSelectedScene == SceneOptions::MarkerDemo)
    {
        w.text("Left-click and move mouse...");
    }
    else if (mSelectedScene == SceneOptions::VoxelNormals)
    {
        w.text("Left-click and move mouse in the left boxes to display the normal there.");
        w.checkbox("Show normal field", mVoxelNormalsGUI.showNormalField, false);
        w.checkbox("Show boxes", mVoxelNormalsGUI.showBoxes, false);
        w.checkbox("Show box diagonals", mVoxelNormalsGUI.showBoxDiagonals, false);
        w.checkbox("Show border lines", mVoxelNormalsGUI.showBorderLines, false);
        w.checkbox("Show box around point", mVoxelNormalsGUI.showBoxAroundPoint, false);
    }
}


void CausticVolumes::createPipelines(RenderContext* pRenderContext)
{
    Dictionary dict;
    RenderPassLibrary::instance().loadLibrary("GBuffer.dll");
    mGBufferPass = RenderPassLibrary::instance().createPass(pRenderContext, "GBufferRaster", dict);
    mGBufferPass->setScene(pRenderContext, mpScene);

    mpSSAOPass = RenderPassLibrary::instance().createPass(pRenderContext, "SSAO", dict);
    mpSSAOPass->setScene(pRenderContext, mpScene);

    mpCausticsVolumeRG = RenderGraph::create("ARPASptPipeline");
    mpCausticsVolumeRG->addPass(mGBufferPass, "GBuffer");
    mpCausticsVolumeRG->markOutput("GBuffer.faceNormalW");
    mpCausticsVolumeRG->compile(pRenderContext);

    mpSSAORG = RenderGraph::create("SSAOPipeline");
    mpSSAORG->addPass(mGBufferPass, "GBuffer");
    mpSSAORG->addPass(mpSSAOPass, "SSAO");
    mpSSAORG->addEdge("GBuffer.diffuseOpacity", "SSAO.colorIn");
    mpSSAORG->addEdge("GBuffer.depth", "SSAO.depth");
    mpSSAORG->addEdge("GBuffer.faceNormalW", "SSAO.normals");
    mpSSAORG->markOutput("SSAO.colorOut");
    mpSSAORG->compile(pRenderContext);
}

void CausticVolumes::createRenderPass()
{
    switch (mSelectedScene)
    {
    case SceneOptions::MarkerDemo:
        mpVisualisationPass = FullScreenPass::create(kMarkerShaderFile);
        break;
    case SceneOptions::VoxelNormals:
        mpVisualisationPass = FullScreenPass::create(kNormalsShaderFile);
        break;
    default:
        mpVisualisationPass = nullptr;
        break;
    }
}


void CausticVolumes::loadScene(const std::string& filename, const Fbo* pTargetFbo)
{
    mpScene = Scene::create(filename);
    if (!mpScene) return;

    mpCamera = mpScene->getCamera();

    // Update the controllers
    float radius = mpScene->getSceneBounds().radius();
    mpScene->setCameraSpeed(radius * 0.25f);
    float nearZ = std::max(0.1f, radius / 750.0f);
    float farZ = radius * 10;
    mpCamera->setDepthRange(nearZ, farZ);
    mpCamera->setAspectRatio((float)pTargetFbo->getWidth() / (float)pTargetFbo->getHeight());

    

    //mpRasterPass = RasterScenePass::create(mpScene, "Samples/HelloDXR/HelloDXR.ps.slang", "", "main");

    //RtProgram::Desc rtProgDesc;
    //rtProgDesc.addShaderLibrary("Samples/HelloDXR/HelloDXR.rt.slang").setRayGen("rayGen");
    //rtProgDesc.addHitGroup(0, "primaryClosestHit", "primaryAnyHit").addMiss(0, "primaryMiss");
    //rtProgDesc.addHitGroup(1, "", "shadowAnyHit").addMiss(1, "shadowMiss");
    //rtProgDesc.addDefines(mpScene->getSceneDefines());
    //rtProgDesc.setMaxTraceRecursionDepth(3); // 1 for calling TraceRay from RayGen, 1 for calling it from the primary-ray ClosestHitShader for reflections, 1 for reflection ray tracing a shadow ray

    //mpRaytraceProgram = RtProgram::create(rtProgDesc);
    //mpRtVars = RtProgramVars::create(mpRaytraceProgram, mpScene);
    //mpRaytraceProgram->setScene(mpScene);
}


void CausticVolumes::onLoad(RenderContext* pRenderContext)
{
    if (gpDevice->isFeatureSupported(Device::SupportedFeatures::Raytracing) == false)
    {
        logFatal("Device does not support raytracing!");
    }

    loadScene(kDefaultScene, gpFramework->getTargetFbo().get());
}

void CausticVolumes::onFrameRender(RenderContext* pRenderContext, const Fbo::SharedPtr& pTargetFbo)
{
    if (!mpCausticsVolumeRG)
    {
        createPipelines(pRenderContext);
    }

    mpScene->update(pRenderContext, gpFramework->getGlobalClock().getTime());


    const float4 clearColor(0.38f, 0.52f, 0.10f, 1);
    pRenderContext->clearFbo(pTargetFbo.get(), clearColor, 1.0f, 0, FboAttachmentType::All);

    if (mShowCVPipeline)
    {
        mpCausticsVolumeRG->execute(pRenderContext);
        pRenderContext->blit(mpCausticsVolumeRG->getOutput("GBuffer.faceNormalW")->getSRV(), pTargetFbo->getRenderTargetView(0));
    }
    else
    {
        mpSSAORG->execute(pRenderContext);
        pRenderContext->blit(mpSSAORG->getOutput("SSAO.colorOut")->getSRV(), pTargetFbo->getRenderTargetView(0));
        if (mpVisualisationPass)
        {
            float width = (float)pTargetFbo->getWidth();
            float height = (float)pTargetFbo->getHeight();
            mpVisualisationPass["Visual2DCB"]["iResolution"] = float2(width, height);
            mpVisualisationPass["Visual2DCB"]["iGlobalTime"] = (float)gpFramework->getGlobalClock().getTime();
            mpVisualisationPass["Visual2DCB"]["iMousePosition"] = mMousePosition;

            switch (mSelectedScene)
            {
            case SceneOptions::MarkerDemo:
                break;
            case SceneOptions::VoxelNormals:
                mpVisualisationPass["VoxelNormalsCB"]["iShowNormalField"] = mVoxelNormalsGUI.showNormalField;
                mpVisualisationPass["VoxelNormalsCB"]["iShowBoxes"] = mVoxelNormalsGUI.showBoxes;
                mpVisualisationPass["VoxelNormalsCB"]["iShowBoxDiagonals"] = mVoxelNormalsGUI.showBoxDiagonals;
                mpVisualisationPass["VoxelNormalsCB"]["iShowBorderLines"] = mVoxelNormalsGUI.showBorderLines;
                mpVisualisationPass["VoxelNormalsCB"]["iShowBoxAroundPoint"] = mVoxelNormalsGUI.showBoxAroundPoint;
                break;
            default:
                break;
            }
            mpVisualisationPass->execute(pRenderContext, pTargetFbo);
        }
    }
}

void CausticVolumes::onShutdown()
{
}

bool CausticVolumes::onKeyEvent(const KeyboardEvent& keyEvent)
{
    if (mpScene && mpScene->onKeyEvent(keyEvent))
        return true;

    if (keyEvent.key == KeyboardEvent::Key::Space && keyEvent.type == KeyboardEvent::Type::KeyReleased)
    {
        mShowCVPipeline = !mShowCVPipeline;
    }

    return false;
}

bool CausticVolumes::onMouseEvent(const MouseEvent& mouseEvent)
{
    bool bHandled = false;
    switch (mouseEvent.type)
    {
    case MouseEvent::Type::LeftButtonDown:
        mLeftButtonDown = true;
        bHandled = true;
        break;
    case MouseEvent::Type::LeftButtonUp:
        mLeftButtonDown = false;
        bHandled = true;
        break;
    case MouseEvent::Type::Move:
        if (mLeftButtonDown)
        {
            mMousePosition = mouseEvent.screenPos;
            bHandled = true;
        }
        break;
    default:
        break;
    }
    return bHandled;


    //if (mpScene && mpScene->onMouseEvent(mouseEvent)) return true;
    //return false;
}

void CausticVolumes::onHotReload(HotReloadFlags reloaded)
{
}

void CausticVolumes::onResizeSwapChain(uint32_t width, uint32_t height)
{
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    CausticVolumes::UniquePtr pRenderer = std::make_unique<CausticVolumes>();
    SampleConfig config;
    config.windowDesc.title = "Falcor Project Template";
    config.windowDesc.resizableWindow = true;
    Sample::run(config, pRenderer);
    return 0;
}
