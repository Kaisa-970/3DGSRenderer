#include "PlyLoader.h"
#include "Logger/Log.h"
#include "tinyply/tinyply.h"
#include <fstream>

using namespace tinyply;
RENDERER_NAMESPACE_BEGIN

inline std::vector<uint8_t> read_file_binary(const std::string &pathToFile)
{
    std::ifstream file(pathToFile, std::ios::binary);
    std::vector<uint8_t> fileBufferBytes;

    if (file.is_open())
    {
        file.seekg(0, std::ios::end);
        size_t sizeBytes = file.tellg();
        file.seekg(0, std::ios::beg);
        fileBufferBytes.resize(sizeBytes);
        if (file.read((char *)fileBufferBytes.data(), sizeBytes))
            return fileBufferBytes;
    }
    else
        throw std::runtime_error("could not open binary ifstream to path " + pathToFile);
    return fileBufferBytes;
}

PlyLoader::PlyLoader()
{
}

PlyLoader::~PlyLoader()
{
}

bool PlyLoader::LoadGaussianPlyAoS(const std::string &filepath, std::vector<GaussianPoint<0>> &gaussianPoints)
{
    LOG_CORE_INFO("........................................................................\n");
    LOG_CORE_INFO("Now Reading: {}", filepath);

    std::unique_ptr<std::istream> file_stream;
    std::vector<uint8_t> byte_buffer;

    try
    {
        bool preload_into_memory = false;
        // For most files < 1gb, pre-loading the entire file upfront and wrapping it into a
        // stream is a net win for parsing speed, about 40% faster.
        if (preload_into_memory)
        {
            byte_buffer = read_file_binary(filepath);
            // file_stream.reset(new memory_stream((char *)byte_buffer.data(), byte_buffer.size()));
        }
        else
        {
            file_stream.reset(new std::ifstream(filepath, std::ios::binary));
        }

        if (!file_stream || file_stream->fail())
        {
            LOG_CORE_ERROR("file_stream failed to open {}", filepath);
            return false;
        }

        file_stream->seekg(0, std::ios::end);
        const float size_mb = file_stream->tellg() * float(1e-6);
        file_stream->seekg(0, std::ios::beg);

        PlyFile file;
        file.parse_header(*file_stream);

        LOG_CORE_INFO("\t[ply_header] Type: {}", (file.is_binary_file() ? "binary" : "ascii"));
        for (const auto &c : file.get_comments())
            LOG_CORE_INFO("\t[ply_header] Comment: {}", c);
        for (const auto &c : file.get_info())
            LOG_CORE_INFO("\t[ply_header] Info: {}", c);

        for (const auto &e : file.get_elements())
        {
            LOG_CORE_INFO("\t[ply_header] element: {} ({})", e.name, e.size);
            for (const auto &p : e.properties)
            {
                LOG_CORE_INFO("\t[ply_header] \tproperty: {} (type={})", p.name,
                              tinyply::PropertyTable[p.propertyType].str);
                if (p.isList)
                    LOG_CORE_INFO("\t[ply_header] \tproperty: {} (list_type={})", p.name,
                                  tinyply::PropertyTable[p.listType].str);
            }
        }

        // Because most people have their own mesh types, tinyply treats parsed data as structured/typed byte buffers.
        // See examples below on how to marry your own application-specific data structures with this one.
        std::shared_ptr<PlyData> positions, normals, shs, opacitys, scales, rotations;

        // The header information can be used to programmatically extract properties on elements
        // known to exist in the header prior to reading the data. For brevity of this sample, properties
        // like vertex position are hard-coded:
        try
        {
            positions = file.request_properties_from_element("vertex", {"x", "y", "z"});
            shs = file.request_properties_from_element("vertex", {"f_dc_0", "f_dc_1", "f_dc_2"});
            opacitys = file.request_properties_from_element("vertex", {"opacity"});
            scales = file.request_properties_from_element("vertex", {"scale_0", "scale_1", "scale_2"});
            rotations = file.request_properties_from_element("vertex", {"rot_0", "rot_1", "rot_2", "rot_3"});
        }
        catch (const std::exception &e)
        {
            LOG_CORE_ERROR("tinyply exception: {}", e.what());
            return false;
        }

        try
        {
            normals = file.request_properties_from_element("vertex", {"nx", "ny", "nz"});
        }
        catch (const std::exception &e)
        {
            LOG_CORE_WARN("Normal attributes not found, will use default values");
            normals = nullptr;
        }

        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
        file.read(*file_stream);
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        std::chrono::duration<double> duration = end - start;
        LOG_CORE_INFO("Read time: {} seconds", duration.count());

        start = std::chrono::steady_clock::now();
        if (positions && shs && opacitys && scales && rotations)
        {
            unsigned int count = positions->count;
            gaussianPoints.resize(count);
            LOG_CORE_INFO("Load {} points", count);
            const float *positionsData = reinterpret_cast<const float *>(positions->buffer.get());
            const float *shsData = reinterpret_cast<const float *>(shs->buffer.get());
            const float *opacitysData = reinterpret_cast<const float *>(opacitys->buffer.get());
            const float *scalesData = reinterpret_cast<const float *>(scales->buffer.get());
            const float *rotationsData = reinterpret_cast<const float *>(rotations->buffer.get());
            for (unsigned int i = 0; i < count; i++)
            {
                GaussianPoint<0> &gaussianPoint = gaussianPoints[i];
                unsigned int offset = i * 3;
                gaussianPoint.position.position[0] = positionsData[offset];
                gaussianPoint.position.position[1] = positionsData[offset + 1];
                gaussianPoint.position.position[2] = positionsData[offset + 2];
                gaussianPoint.shs.shs[0] = shsData[offset];
                gaussianPoint.shs.shs[1] = shsData[offset + 1];
                gaussianPoint.shs.shs[2] = shsData[offset + 2];
                gaussianPoint.opacity.opacity = opacitysData[i];
                gaussianPoint.scale.scale[0] = scalesData[offset];
                gaussianPoint.scale.scale[1] = scalesData[offset + 1];
                gaussianPoint.scale.scale[2] = scalesData[offset + 2];
                gaussianPoint.rotation.rotation[0] = rotationsData[offset + i];
                gaussianPoint.rotation.rotation[1] = rotationsData[offset + i + 1];
                gaussianPoint.rotation.rotation[2] = rotationsData[offset + i + 2];
                gaussianPoint.rotation.rotation[3] = rotationsData[offset + i + 3];
            }

            if (normals)
            {
                const float *normalsData = reinterpret_cast<const float *>(normals->buffer.get());

                for (unsigned int i = 0; i < count; i++)
                {
                    GaussianPoint<0> &gaussianPoint = gaussianPoints[i];
                    unsigned int offset = i * 3;
                    gaussianPoint.normal.normal[0] = normalsData[offset];
                    gaussianPoint.normal.normal[1] = normalsData[offset + 1];
                    gaussianPoint.normal.normal[2] = normalsData[offset + 2];
                }
            }
            end = std::chrono::steady_clock::now();
            duration = end - start;
            LOG_CORE_INFO("Load points time: {} seconds", duration.count());
        }
        else
        {
            LOG_CORE_ERROR("Failed to load GaussianPly: missing required properties");
            return false;
        }

        return true;
    }
    catch (const std::exception &e)
    {
        LOG_CORE_ERROR("Caught tinyply exception: {}", e.what());
        return false;
    }
}

bool PlyLoader::LoadGaussianPlySoA(const std::string &filepath, GaussianData<0> &gaussianData)
{
    LOG_CORE_INFO("........................................................................\n");
    LOG_CORE_INFO("Now Reading: {}", filepath);

    std::unique_ptr<std::istream> file_stream;
    std::vector<uint8_t> byte_buffer;

    try
    {
        bool preload_into_memory = false;
        // For most files < 1gb, pre-loading the entire file upfront and wrapping it into a
        // stream is a net win for parsing speed, about 40% faster.
        if (preload_into_memory)
        {
            byte_buffer = read_file_binary(filepath);
            // file_stream.reset(new memory_stream((char *)byte_buffer.data(), byte_buffer.size()));
        }
        else
        {
            file_stream.reset(new std::ifstream(filepath, std::ios::binary));
        }

        if (!file_stream || file_stream->fail())
        {
            LOG_CORE_ERROR("file_stream failed to open {}", filepath);
            return false;
        }

        // file_stream->seekg(0, std::ios::end);
        // const float size_mb = file_stream->tellg() * float(1e-6);
        // file_stream->seekg(0, std::ios::beg);

        PlyFile file;
        file.parse_header(*file_stream);

        LOG_CORE_INFO("\t[ply_header] Type: {}", (file.is_binary_file() ? "binary" : "ascii"));
        for (const auto &c : file.get_comments())
            LOG_CORE_INFO("\t[ply_header] Comment: {}", c);
        for (const auto &c : file.get_info())
            LOG_CORE_INFO("\t[ply_header] Info: {}", c);

        for (const auto &e : file.get_elements())
        {
            LOG_CORE_INFO("\t[ply_header] element: {} ({})", e.name, e.size);
            for (const auto &p : e.properties)
            {
                LOG_CORE_INFO("\t[ply_header] \tproperty: {} (type={})", p.name,
                              tinyply::PropertyTable[p.propertyType].str);
                if (p.isList)
                    LOG_CORE_INFO("\t[ply_header] \tproperty: {} (list_type={})", p.name,
                                  tinyply::PropertyTable[p.listType].str);
            }
        }

        // Because most people have their own mesh types, tinyply treats parsed data as structured/typed byte buffers.
        // See examples below on how to marry your own application-specific data structures with this one.
        std::shared_ptr<PlyData> positions, normals, shs, opacitys, scales, rotations;

        // The header information can be used to programmatically extract properties on elements
        // known to exist in the header prior to reading the data. For brevity of this sample, properties
        // like vertex position are hard-coded:
        try
        {
            positions = file.request_properties_from_element("vertex", {"x", "y", "z"});
            shs = file.request_properties_from_element("vertex", {"f_dc_0", "f_dc_1", "f_dc_2"});
            opacitys = file.request_properties_from_element("vertex", {"opacity"});
            scales = file.request_properties_from_element("vertex", {"scale_0", "scale_1", "scale_2"});
            rotations = file.request_properties_from_element("vertex", {"rot_0", "rot_1", "rot_2", "rot_3"});
        }
        catch (const std::exception &e)
        {
            LOG_CORE_ERROR("tinyply exception: {}", e.what());
            return false;
        }

        try
        {
            normals = file.request_properties_from_element("vertex", {"nx", "ny", "nz"});
        }
        catch (const std::exception &e)
        {
            LOG_CORE_WARN("Normal attributes not found, will use default values");
            normals = nullptr;
        }

        file.read(*file_stream);
        if (positions)
        {
            const float *positionsData = reinterpret_cast<const float *>(positions->buffer.get());
            gaussianData.positions.resize(positions->count);
            std::memcpy(gaussianData.positions.data(), positionsData, positions->count * sizeof(GSPosition));
        }
        if (normals)
        {
            const float *normalsData = reinterpret_cast<const float *>(normals->buffer.get());
            gaussianData.normals.resize(normals->count);
            std::memcpy(gaussianData.normals.data(), normalsData, normals->count * sizeof(GSNormal));
        }
        if (shs)
        {
            const float *shsData = reinterpret_cast<const float *>(shs->buffer.get());
            gaussianData.shs.resize(shs->count);
            std::memcpy(gaussianData.shs.data(), shsData, shs->count * sizeof(SHs<0>));
        }
        if (opacitys)
        {
            const float *opacitysData = reinterpret_cast<const float *>(opacitys->buffer.get());
            gaussianData.opacitys.resize(opacitys->count);
            std::memcpy(gaussianData.opacitys.data(), opacitysData, opacitys->count * sizeof(GSOpacity));
        }
        if (scales)
        {
            const float *scalesData = reinterpret_cast<const float *>(scales->buffer.get());
            gaussianData.scales.resize(scales->count);
            std::memcpy(gaussianData.scales.data(), scalesData, scales->count * sizeof(GSScale));
        }
        if (rotations)
        {
            const float *rotationsData = reinterpret_cast<const float *>(rotations->buffer.get());
            gaussianData.rotations.resize(rotations->count);
            std::memcpy(gaussianData.rotations.data(), rotationsData, rotations->count * sizeof(GSRotation));
        }

        return true;
    }
    catch (const std::exception &e)
    {
        LOG_CORE_ERROR("Caught tinyply exception: {}", e.what());
        return false;
    }
}

RENDERER_NAMESPACE_END
