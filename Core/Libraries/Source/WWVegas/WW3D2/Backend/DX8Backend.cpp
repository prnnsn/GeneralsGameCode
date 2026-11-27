/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2026 TheSuperHackers
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// TheSuperHackers @refactor bobtista 08/10/2026 DX8 definition of the Renderer
// methods. Init and Shutdown own the DX8Wrapper lifecycle. The rendering methods
// are one-line trampolines to the existing DX8Wrapper static API.

#include "WW3D2/Renderer.h"

#include "WW3D2/Backend/DX8ManagedBufferWrite.h"
#include "WW3D2/Backend/DX8StreamingBufferWrite.h"
#include "WW3D2/Backend/DX8VertexLayout.h"
#include "WW3D2/dx8wrapper.h"
#include "WW3D2/formconv.h"
#include "WWMath/vector3.h"
#include "WW3D2/lightenvironment.h"
#include "WWDebug/wwdebug.h"

static D3DTRANSFORMSTATETYPE To_D3D_Transform(RenderTransform transform)
{
    switch (transform)
    {
    case RenderTransform_World:
        return D3DTS_WORLD;
    case RenderTransform_View:
        return D3DTS_VIEW;
    case RenderTransform_Projection:
        return D3DTS_PROJECTION;
    }
    WWASSERT(0);
    return D3DTS_WORLD;
}

static bool Initialized = false;
static bool Lite = false;

bool Renderer::Init(void * window, bool lite)
{
    WWASSERT(!Initialized);
    if (Initialized)
    {
        return true;
    }

    Init_D3D_To_WW3_Conversion();
    WWDEBUG_SAY(("Init DX8Wrapper"));
    if (!DX8Wrapper::Init(window, lite))
    {
        return false;
    }

    Initialized = true;
    Lite = lite;
    return true;
}

void Renderer::Shutdown()
{
    if (Initialized && !Lite)
    {
        DX8Wrapper::Shutdown();
    }
    Initialized = false;
}

int Renderer::Get_Render_Device_Count()
{
    return DX8Wrapper::Get_Render_Device_Count();
}

int Renderer::Get_Render_Device()
{
    return DX8Wrapper::Get_Render_Device();
}

const char * Renderer::Get_Render_Device_Name(int device_index)
{
    return DX8Wrapper::Get_Render_Device_Name(device_index);
}

void Renderer::Get_Device_Resolution(int & width, int & height, int & bits, bool & windowed)
{
    DX8Wrapper::Get_Device_Resolution(width, height, bits, windowed);
}

void Renderer::Get_Render_Target_Resolution(int & width, int & height, int & bits, bool & windowed)
{
    DX8Wrapper::Get_Render_Target_Resolution(width, height, bits, windowed);
}

int Renderer::Get_Device_Resolution_Width()
{
    return DX8Wrapper::Get_Device_Resolution_Width();
}

int Renderer::Get_Device_Resolution_Height()
{
    return DX8Wrapper::Get_Device_Resolution_Height();
}

bool Renderer::Is_Windowed()
{
    return DX8Wrapper::Is_Windowed();
}

int Renderer::Get_Texture_Bitdepth()
{
    return DX8Wrapper::Get_Texture_Bitdepth();
}

int Renderer::Get_Swap_Interval()
{
    return DX8Wrapper::Get_Swap_Interval();
}

bool Renderer::Has_Stencil()
{
    return DX8Wrapper::Has_Stencil();
}

WW3DFormat Renderer::Get_Back_Buffer_Format()
{
    return DX8Wrapper::getBackBufferFormat();
}

void Renderer::Set_Gamma(float gamma, float bright, float contrast, bool calibrate, bool uselimit)
{
    DX8Wrapper::Set_Gamma(gamma, bright, contrast, calibrate, uselimit);
}

void Renderer::Begin_Scene()
{
    DX8Wrapper::Begin_Scene();
}

void Renderer::End_Scene(bool flip_frame)
{
    DX8Wrapper::End_Scene(flip_frame);
}

void Renderer::Flip_To_Primary()
{
    DX8Wrapper::Flip_To_Primary();
}

void Renderer::Clear(bool clear_color, bool clear_z_stencil,
                     const Vector3 & color,
                     float dest_alpha, float z, unsigned int stencil)
{
    DX8Wrapper::Clear(clear_color, clear_z_stencil, color, dest_alpha, z, stencil);
}

void Renderer::Set_Viewport(const RenderViewport & viewport)
{
    D3DVIEWPORT8 vp;
    vp.X      = viewport.x;
    vp.Y      = viewport.y;
    vp.Width  = viewport.width;
    vp.Height = viewport.height;
    vp.MinZ   = viewport.min_z;
    vp.MaxZ   = viewport.max_z;
    DX8Wrapper::Set_Viewport(&vp);
}

void Renderer::Invalidate_Cached_Render_States()
{
    DX8Wrapper::Invalidate_Cached_Render_States();
}

void Renderer::Set_Shader(const ShaderClass & shader)
{
    DX8Wrapper::Set_Shader(shader);
}

void Renderer::Set_Texture(unsigned stage, TextureBaseClass * texture)
{
    DX8Wrapper::Set_Texture(stage, texture);
}

static DWORD To_D3D_Texture_Address(Renderer::TextureAddress address)
{
    switch (address)
    {
    case Renderer::TextureAddress::Wrap: return D3DTADDRESS_WRAP;
    case Renderer::TextureAddress::Mirror: return D3DTADDRESS_MIRROR;
    case Renderer::TextureAddress::Clamp: return D3DTADDRESS_CLAMP;
    case Renderer::TextureAddress::Border: return D3DTADDRESS_BORDER;
    case Renderer::TextureAddress::MirrorOnce: return D3DTADDRESS_MIRRORONCE;
    }
    WWASSERT(0);
    return D3DTADDRESS_WRAP;
}

void Renderer::Set_Texture_Address_U(unsigned stage, TextureAddress address)
{
    DX8Wrapper::Set_DX8_Texture_Stage_State(stage, D3DTSS_ADDRESSU, To_D3D_Texture_Address(address));
}

void Renderer::Set_Texture_Address_V(unsigned stage, TextureAddress address)
{
    DX8Wrapper::Set_DX8_Texture_Stage_State(stage, D3DTSS_ADDRESSV, To_D3D_Texture_Address(address));
}

void Renderer::Set_Texture_Border_Color(unsigned stage, unsigned argb)
{
    DX8Wrapper::Set_DX8_Texture_Stage_State(stage, D3DTSS_BORDERCOLOR, argb);
}

static DWORD To_D3D_Texture_Filter(Renderer::TextureFilter filter)
{
    switch (filter)
    {
    case Renderer::TextureFilter::None: return D3DTEXF_NONE;
    case Renderer::TextureFilter::Point: return D3DTEXF_POINT;
    case Renderer::TextureFilter::Linear: return D3DTEXF_LINEAR;
    case Renderer::TextureFilter::Anisotropic: return D3DTEXF_ANISOTROPIC;
    }
    WWASSERT(0);
    return D3DTEXF_POINT;
}

Renderer::TextureFilterCaps Renderer::Get_Texture_Filter_Caps()
{
    const DWORD caps = DX8Wrapper::Get_Current_Caps()->Get_DX8_Caps().TextureFilterCaps;
    return {
        (caps & D3DPTFILTERCAPS_MINFLINEAR) != 0,
        (caps & D3DPTFILTERCAPS_MAGFLINEAR) != 0,
        (caps & D3DPTFILTERCAPS_MIPFLINEAR) != 0,
        (caps & D3DPTFILTERCAPS_MINFANISOTROPIC) != 0,
        (caps & D3DPTFILTERCAPS_MAGFANISOTROPIC) != 0
    };
}

void Renderer::Set_Texture_Min_Filter(unsigned stage, TextureFilter filter)
{
    DX8Wrapper::Set_DX8_Texture_Stage_State(stage, D3DTSS_MINFILTER, To_D3D_Texture_Filter(filter));
}

void Renderer::Set_Texture_Mag_Filter(unsigned stage, TextureFilter filter)
{
    DX8Wrapper::Set_DX8_Texture_Stage_State(stage, D3DTSS_MAGFILTER, To_D3D_Texture_Filter(filter));
}

void Renderer::Set_Texture_Mip_Filter(unsigned stage, TextureFilter filter)
{
    DX8Wrapper::Set_DX8_Texture_Stage_State(stage, D3DTSS_MIPFILTER, To_D3D_Texture_Filter(filter));
}

void Renderer::Set_Texture_Max_Anisotropy(unsigned stage, unsigned level)
{
    DX8Wrapper::Set_DX8_Texture_Stage_State(stage, D3DTSS_MAXANISOTROPY, level);
}

void Renderer::Set_Material(const VertexMaterialClass * material)
{
    DX8Wrapper::Set_Material(material);
}

void Renderer::Apply_Render_State_Changes()
{
    DX8Wrapper::Apply_Render_State_Changes();
}

bool Renderer::Try_Begin_Terrain_Depth_Bias(unsigned strength)
{
    if (strength == 0 || strength > 16 || !DX8Wrapper::_Get_D3D_Device8() ||
        !DX8Wrapper::Get_Current_Caps()->Support_ZBias())
        return false;

    if (DX8Wrapper::Get_DX8_Render_State(D3DRS_ZBIAS) != 0)
        return false;

    DX8Wrapper::Set_DX8_Render_State(D3DRS_ZBIAS, strength);
    return true;
}

void Renderer::End_Terrain_Depth_Bias()
{
    DX8Wrapper::Set_DX8_Render_State(D3DRS_ZBIAS, 0);
}

void Renderer::Draw_Triangles(unsigned short start_index, unsigned short polygon_count,
                              unsigned short min_vertex_index, unsigned short vertex_count)
{
    DX8Wrapper::Draw_Triangles(start_index, polygon_count, min_vertex_index, vertex_count);
}

void Renderer::Set_Vertex_Buffer(const VertexBufferClass * vb, unsigned stream)
{
    DX8Wrapper::Set_Vertex_Buffer(vb, stream);
}

void Renderer::Set_Vertex_Buffer(const DynamicVBAccessClass & vba)
{
    DX8Wrapper::Set_Vertex_Buffer(vba);
}

void Renderer::Set_Index_Buffer(const IndexBufferClass * ib, unsigned short index_base_offset)
{
    DX8Wrapper::Set_Index_Buffer(ib, index_base_offset);
}

void Renderer::Set_Index_Buffer(const DynamicIBAccessClass & iba, unsigned short index_base_offset)
{
    DX8Wrapper::Set_Index_Buffer(iba, index_base_offset);
}

void Renderer::Set_Index_Buffer_Index_Offset(unsigned offset)
{
    DX8Wrapper::Set_Index_Buffer_Index_Offset(offset);
}

static bool Can_Create_Rewritable_Buffer(const RenderBuffers::BufferDesc & desc, RenderBuffers::Role role)
{
    using namespace RenderBuffers;
    std::uint32_t bytes;
    return DX8Wrapper::_Get_D3D_Device8() && desc.role == role && desc.storage == Storage::Gpu &&
           desc.update == Update::Rewritable && desc.recreation == Recreation::Regenerate &&
           TryBufferSize(desc, bytes);
}

VertexBufferClass * Renderer::Create_Rewritable_Vertex_Buffer(const RenderBuffers::BufferDesc & desc)
{
    using namespace RenderBuffers;
    if (!Can_Create_Rewritable_Buffer(desc, Role::Vertex))
        return nullptr;

    // XYZDUV1 is the first migrated producer. Compare the layout before passing its FVF to DX8.
    VertexLayout expected;
    if (!DX8VertexLayout::TryDecode(DX8_FVF_XYZDUV1, desc.stride_bytes, expected) ||
        desc.layout.count != expected.count)
        return nullptr;
    for (unsigned i = 0; i < expected.count; ++i)
    {
        const Attribute & actual = desc.layout.attributes[i];
        const Attribute & native = expected.attributes[i];
        if (actual.semantic != native.semantic || actual.semantic_index != native.semantic_index ||
            actual.type != native.type || actual.byte_offset != native.byte_offset)
            return nullptr;
    }

    auto * buffer = NEW_REF(DX8VertexBufferClass, (DX8_FVF_XYZDUV1, static_cast<unsigned short>(desc.element_count),
                                                  DX8VertexBufferClass::USAGE_DEFAULT, false));
    if (!buffer->Get_DX8_Vertex_Buffer())
        REF_PTR_RELEASE(buffer);
    return buffer;
}

IndexBufferClass * Renderer::Create_Rewritable_Index_Buffer(const RenderBuffers::BufferDesc & desc)
{
    if (!Can_Create_Rewritable_Buffer(desc, RenderBuffers::Role::Index))
        return nullptr;
    auto * buffer = NEW_REF(DX8IndexBufferClass, (static_cast<unsigned short>(desc.element_count),
                                                 DX8IndexBufferClass::USAGE_DEFAULT, false));
    if (!buffer->Get_DX8_Index_Buffer())
        REF_PTR_RELEASE(buffer);
    return buffer;
}

bool Renderer::Upload_Dynamic_Vertices(DynamicVBAccessClass & access, const void * source, unsigned stride)
{
    auto * buffer = access.VertexBuffer;
    if (access.Type != BUFFER_TYPE_DYNAMIC_DX8 || !buffer || !DX8Wrapper::_Get_D3D_Device8() ||
        !source || stride != buffer->FVF_Info().Get_FVF_Size())
        return false;
    DX8_THREAD_ASSERT();
    DX8_Assert();
    auto * native = static_cast<DX8VertexBufferClass *>(buffer)->Get_DX8_Vertex_Buffer();
    const bool uploaded = DX8StreamingBufferWrite::Upload(native, buffer->Get_Vertex_Count(), stride,
        access.VertexBufferOffset, access.VertexCount, source, D3DLOCK_NOSYSLOCK);
    if (!uploaded)
        access.Retire_DX8_Buffer();
    return uploaded;
}

bool Renderer::Upload_Dynamic_Indices(DynamicIBAccessClass & access, const unsigned short * source)
{
    auto * buffer = access.IndexBuffer;
    if (access.Type != BUFFER_TYPE_DYNAMIC_DX8 || !buffer || !DX8Wrapper::_Get_D3D_Device8() || !source)
        return false;
    DX8_THREAD_ASSERT();
    DX8_Assert();
    auto * native = static_cast<DX8IndexBufferClass *>(buffer)->Get_DX8_Index_Buffer();
    buffer->Add_Ref();
    const bool uploaded = DX8StreamingBufferWrite::Upload(native, buffer->Get_Index_Count(), sizeof(WORD),
        access.IndexBufferOffset, access.IndexCount, source, 0);
    if (!uploaded)
        access.Retire_DX8_Buffer();
    buffer->Release_Ref();
    return uploaded;
}

bool Renderer::Bind_Dynamic_Triangle_List(const DynamicVBAccessClass & vertices, const DynamicIBAccessClass & indices)
{
    if (vertices.Type != BUFFER_TYPE_DYNAMIC_DX8 || indices.Type != BUFFER_TYPE_DYNAMIC_DX8 ||
        !vertices.VertexBuffer || !indices.IndexBuffer || indices.IndexCount % 3 != 0)
        return false;
    RenderBuffers::DrawOffsets offsets;
    if (!RenderBuffers::TryDrawOffsets(vertices.VertexBufferOffset, 0, indices.IndexBufferOffset, 0, offsets) ||
        !RenderBuffers::ValidIndexedDraw(offsets, RenderBuffers::Topology::TriangleList, indices.IndexCount / 3,
            indices.IndexBuffer->Get_Index_Count(), 0, vertices.VertexCount, vertices.VertexBuffer->Get_Vertex_Count()))
        return false;
    Set_Index_Buffer(indices, 0);
    Set_Vertex_Buffer(vertices);
    return true;
}

Renderer::BufferWrite::BufferWrite(VertexBufferClass * buffer, unsigned first, unsigned count)
{
    if (!buffer || buffer->Type() != BUFFER_TYPE_DX8 || !DX8Wrapper::_Get_D3D_Device8())
        return;
    DX8_THREAD_ASSERT();
    WWASSERT(!buffer->Engine_Refs());
    DX8_Assert();
    buffer->Add_Ref();
    data = DX8ManagedBufferWrite::Lock(static_cast<DX8VertexBufferClass *>(buffer)->Get_DX8_Vertex_Buffer(),
        buffer->Get_Vertex_Count(), buffer->FVF_Info().Get_FVF_Size(), first, count, true);
    if (!data)
    {
        buffer->Release_Ref();
        return;
    }
    owner = buffer;
    finish = [](RefCountClass * ref) {
        return DX8ManagedBufferWrite::Unlock(*static_cast<DX8VertexBufferClass *>(ref)->Get_DX8_Vertex_Buffer());
    };
}

Renderer::BufferWrite::BufferWrite(IndexBufferClass * buffer, unsigned first, unsigned count)
{
    if (!buffer || buffer->Type() != BUFFER_TYPE_DX8 || !DX8Wrapper::_Get_D3D_Device8())
        return;
    DX8_THREAD_ASSERT();
    WWASSERT(!buffer->Engine_Refs());
    DX8_Assert();
    buffer->Add_Ref();
    data = DX8ManagedBufferWrite::Lock(static_cast<DX8IndexBufferClass *>(buffer)->Get_DX8_Index_Buffer(),
        buffer->Get_Index_Count(), sizeof(WORD), first, count, false);
    if (!data)
    {
        buffer->Release_Ref();
        return;
    }
    owner = buffer;
    finish = [](RefCountClass * ref) {
        return DX8ManagedBufferWrite::Unlock(*static_cast<DX8IndexBufferClass *>(ref)->Get_DX8_Index_Buffer());
    };
}

Renderer::BufferWrite::~BufferWrite()
{
    if (owner)
        Finish();
}

bool Renderer::BufferWrite::Finish()
{
    if (!owner)
        return false;
    DX8_THREAD_ASSERT();
    DX8_Assert();
    data = nullptr;
    const bool result = finish(owner);
    REF_PTR_RELEASE(owner);
    return result;
}

void Renderer::Set_Transform(RenderTransform transform, const Matrix4x4 & m)
{
    DX8Wrapper::Set_Transform(To_D3D_Transform(transform), m);
}

void Renderer::Set_Transform(RenderTransform transform, const Matrix3D & m)
{
    DX8Wrapper::Set_Transform(To_D3D_Transform(transform), m);
}

void Renderer::Get_Transform(RenderTransform transform, Matrix4x4 & m)
{
    DX8Wrapper::Get_Transform(To_D3D_Transform(transform), m);
}

void Renderer::Set_World_Identity()
{
    DX8Wrapper::Set_World_Identity();
}

void Renderer::Set_View_Identity()
{
    DX8Wrapper::Set_View_Identity();
}

void Renderer::Set_Projection_Transform_With_Z_Bias(const Matrix4x4 & matrix, float znear, float zfar)
{
    DX8Wrapper::Set_Projection_Transform_With_Z_Bias(matrix, znear, zfar);
}

void Renderer::Set_Ambient(const Vector3 & color)
{
    DX8Wrapper::Set_Ambient(color);
}

void Renderer::Set_Light_Environment(LightEnvironmentClass * light_env)
{
    DX8Wrapper::Set_Light_Environment(light_env);
}

void Renderer::Set_Light(unsigned index, const LightClass & light)
{
    DX8Wrapper::Set_Light(index, light);
}

void Renderer::Clear_Light(unsigned index)
{
    DX8Wrapper::Set_Light(index, nullptr);
}

void Renderer::Set_Fog(bool enable, const Vector3 & color, float start, float end)
{
    DX8Wrapper::Set_Fog(enable, color, start, end);
}
