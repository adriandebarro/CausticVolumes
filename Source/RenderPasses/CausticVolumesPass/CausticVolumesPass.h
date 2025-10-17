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
#pragma once
#include "Falcor.h"
#include "FalcorExperimental.h"
#include <filesystem>
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/error/en.h>
#include "Core/"

using namespace Falcor;

class CausticVolumesPass : public RenderPass
{
public:
    using SharedPtr = std::shared_ptr<CausticVolumesPass>;

    /** Create a new render pass object.
        \param[in] pRenderContext The render context.
        \param[in] dict Dictionary of serialized parameters.
        \return A new object, or an exception is thrown if creation failed.
    */
    static SharedPtr create(RenderContext* pRenderContext = nullptr, const Dictionary& dict = {});

    virtual std::string getDesc() override;
    virtual Dictionary getScriptingDictionary() override;
    virtual RenderPassReflection reflect(const CompileData& compileData) override;
    virtual void compile(RenderContext* pContext, const CompileData& compileData) override {}
    virtual void execute(RenderContext* pRenderContext, const RenderData& renderData) override;
    virtual void renderUI(Gui::Widgets& widget) override;
    virtual void setScene(RenderContext* pRenderContext, const Scene::SharedPtr& pScene) override {}
    virtual bool onMouseEvent(const MouseEvent& mouseEvent) override { return false; }
    virtual bool onKeyEvent(const KeyboardEvent& keyEvent) override { return false; }

private:
    /// <summary>
    /// default copnstructor forn the CV pass
    /// </summary>
    /// <param name="dict"></param>
    CausticVolumesPass(const Dictionary& dict)
    {
        parseDetails(dict);
    }

    /// <summary>
    /// This function parses hte diuctionary which passes the information to the pass
    /// </summary>
    /// <param name="dict"></param>
    void parseDetails(const Dictionary& dict)
    {
        mJsonPath = dict["jsonPath"];

        if (mJsonPath.size() > 0)
        {
            parseJson();
        }
    }

    std::string readFile(const std::filesystem::path& path)
    {
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs)
            throw std::exception(("Failed to read from file " + path.string()).c_str());
        return std::string((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
    }

    bool parseJson()
    {
        // load the json file
        std::filesystem::path jsonFilePath = mJsonPath;

        if (std::filesystem::exists(jsonFilePath))
        {
            std::string jsonData = readFile(jsonFilePath);
            rapidjson::StringStream jsonStream(jsonData.c_str());

            rapidjson::Document jsonDocument;
            jsonDocument.ParseStream(jsonStream);

        }
    }

private:
    std::string mJsonPath = "";

    struct SlabDetails
    {
        uint slabCount;
    };

    struct CausticVolumes
    {
        uint index;
        float3 maxPoint;
        float3 minPoint;
        float3 center;
        bool autoBoundingBox;
        Camera::SharedPtr pSlabsCamera;
        Camera::SharedPtr pProjectorCamera;
    };

    std::vector<CausticVolumes> mBoundingBoxDetails;
};
