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

#include <climits>
#include <memory>

#include "WW3D2/Backend/DX8ManagedBufferWrite.h"
#include "WW3D2/Backend/DX8StreamingBufferWrite.h"
#include "WW3D2/Backend/DX8TextureMipWrite.h"
#include "WW3D2/Backend/DX8VertexLayout.h"
#include "WW3D2/RenderTexture.h"
#include "WW3D2/dx8wrapper.h"
#include "WW3D2/formconv.h"
#include "WW3D2/surfaceclass.h"
#include "WW3D2/texture.h"
#include "WWMath/vector3.h"
#include "WWLib/thread.h"
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

RenderTexture2DHandle Renderer::Create_Load_Texture_2D(unsigned width, unsigned height,
                                                       WW3DFormat format, unsigned mip_levels)
{
    WWASSERT(ThreadClass::_Get_Current_Thread_ID() == DX8Wrapper::_Get_Main_Thread_ID());
    return { DX8Wrapper::_Create_DX8_Texture(width, height, format,
                                            static_cast<MipCountType>(mip_levels), D3DPOOL_MANAGED) };
}

void Renderer::Release_Texture_2D(RenderTexture2DHandle & texture)
{
    WWASSERT(ThreadClass::_Get_Current_Thread_ID() == DX8Wrapper::_Get_Main_Thread_ID());
    if (auto * native = static_cast<IDirect3DTexture8 *>(texture.value))
    {
        native->Release();
        texture = {};
    }
}

void Renderer::Apply_Load_Texture_2D(TextureBaseClass & target, RenderTexture2DHandle & texture,
                                     bool initialize)
{
    WWASSERT(ThreadClass::_Get_Current_Thread_ID() == DX8Wrapper::_Get_Main_Thread_ID());
    WWASSERT(texture && target.Get_Asset_Type() == TextureBaseClass::TEX_REGULAR);
    target.Apply_New_Surface(static_cast<IDirect3DTexture8 *>(texture.value), initialize);
    Release_Texture_2D(texture);
}

unsigned Renderer::Map_Texture_2D_Mips(RenderTexture2DHandle texture, unsigned char ** data,
                                       unsigned * pitch, unsigned capacity)
{
    WWASSERT(ThreadClass::_Get_Current_Thread_ID() == DX8Wrapper::_Get_Main_Thread_ID());
    auto * native = static_cast<IDirect3DTexture8 *>(texture.value);
    return native ? DX8TextureMipWrite::Lock(*native, data, pitch, capacity) : 0;
}

bool Renderer::Unmap_Texture_2D_Mips(RenderTexture2DHandle texture, unsigned char ** data,
                                     unsigned * pitch, unsigned mip_count)
{
    WWASSERT(ThreadClass::_Get_Current_Thread_ID() == DX8Wrapper::_Get_Main_Thread_ID());
    auto * native = static_cast<IDirect3DTexture8 *>(texture.value);
    return DX8TextureMipWrite::Unlock(*native, data, pitch, mip_count);
}

namespace
{
struct DX8SurfaceRelease
{
    void operator()(IDirect3DSurface8 * surface) const { surface->Release(); }
};
using DX8SurfaceRef = std::unique_ptr<IDirect3DSurface8, DX8SurfaceRelease>;

DX8SurfaceRef Acquire_Native_Mip_Surface(const TextureClass & texture, unsigned mip)
{
    auto * native = texture.Peek_D3D_Texture();
    if (!native)
        return {};

    IDirect3DSurface8 * surface = nullptr;
    const HRESULT result = native->GetSurfaceLevel(mip, &surface);
    if (FAILED(result) || !surface)
    {
        Non_Fatal_Log_DX8_ErrorCode(FAILED(result) ? result : E_FAIL, __FILE__, __LINE__);
        return {};
    }

    return DX8SurfaceRef(surface);
}
}

SurfaceClass * Renderer::Get_Texture_2D_Mip_Surface(const TextureClass & texture, unsigned mip)
{
    const auto surface = Acquire_Native_Mip_Surface(texture, mip);
    // The wrapper retains a separate reference before the query reference is released.
    return surface ? new SurfaceClass(surface.get()) : nullptr;
}

bool Renderer::Try_Describe_Texture_2D_Mip(const TextureClass & texture, unsigned mip,
                                            unsigned & width, unsigned & height, WW3DFormat & format)
{
    auto * native = texture.Peek_D3D_Texture();
    if (!native)
        return false;

    D3DSURFACE_DESC desc = {};
    const HRESULT result = native->GetLevelDesc(mip, &desc);
    if (FAILED(result))
    {
        Non_Fatal_Log_DX8_ErrorCode(result, __FILE__, __LINE__);
        return false;
    }

    width = desc.Width;
    height = desc.Height;
    format = D3DFormat_To_WW3DFormat(desc.Format);
    return true;
}

Renderer::TextureMipWrite::~TextureMipWrite()
{
    Finish();
}

bool Renderer::TextureMipWrite::Try_Begin(const TextureClass & texture, unsigned mip)
{
    if (native_surface)
        return false;

    auto surface = Acquire_Native_Mip_Surface(texture, mip);
    if (!surface)
        return false;

    D3DSURFACE_DESC desc = {};
    const HRESULT describe = surface->GetDesc(&desc);
    if (FAILED(describe))
    {
        Non_Fatal_Log_DX8_ErrorCode(describe, __FILE__, __LINE__);
        return false;
    }

    const auto format = D3DFormat_To_WW3DFormat(desc.Format);
    RenderTextures::FormatLayout storage;
    RenderTextures::MipLayout layout;
    if (!RenderTextures::TryFormatLayout(format, storage) ||
        storage.block_width != 1 || storage.block_height != 1 ||
        !RenderTextures::TryMipLayout({ desc.Width, desc.Height, 1, format }, 0, layout))
        return false;

    D3DLOCKED_RECT rect = {};
    const HRESULT lock = surface->LockRect(&rect, nullptr, 0);
    if (FAILED(lock))
    {
        Non_Fatal_Log_DX8_ErrorCode(lock, __FILE__, __LINE__);
        return false;
    }

    std::uint32_t span;
    if (!rect.pBits || rect.Pitch <= 0 ||
        !RenderTextures::TryRowSpan(layout, static_cast<unsigned>(rect.Pitch), INT_MAX, span))
    {
        const HRESULT unlock = surface->UnlockRect();
        Non_Fatal_Log_DX8_ErrorCode(FAILED(unlock) ? unlock : E_FAIL, __FILE__, __LINE__);
        return false;
    }

    view = { static_cast<unsigned char *>(rect.pBits), layout, static_cast<unsigned>(rect.Pitch),
             span, storage.bytes_per_block, format };
    native_surface = surface.release();
    return true;
}

bool Renderer::TextureMipWrite::Finish()
{
    const DX8SurfaceRef surface(static_cast<IDirect3DSurface8 *>(native_surface));
    native_surface = nullptr;
    view = {};
    if (!surface)
        return true;

    const HRESULT result = surface->UnlockRect();
    if (FAILED(result))
        Non_Fatal_Log_DX8_ErrorCode(result, __FILE__, __LINE__);
    return SUCCEEDED(result);
}

bool Renderer::Clear_Texture_2D_Mip(const TextureClass & texture, unsigned mip)
{
    TextureMipWrite write;
    if (!write.Try_Begin(texture, mip))
        return false;

    const auto & mapped = write.View();
    const bool cleared = RenderTextures::TryClearRows(
        mapped.layout, mapped.data, mapped.pitch, mapped.byte_count);
    return write.Finish() && cleared;
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


namespace
{
void Write_Render_State(D3DRENDERSTATETYPE state, DWORD value, Renderer::StateWrite mode)
{
    if (mode == Renderer::StateWrite::Cached)
        DX8Wrapper::Set_DX8_Render_State(state,value);
    else
        DX8Wrapper::_Get_D3D_Device8()->SetRenderState(state,value);
}
DWORD To_D3D_Compare(Renderer::Compare value)
{
    switch (value)
    {
    case Renderer::Compare::Never: return D3DCMP_NEVER;
    case Renderer::Compare::Less: return D3DCMP_LESS;
    case Renderer::Compare::Equal: return D3DCMP_EQUAL;
    case Renderer::Compare::LessEqual: return D3DCMP_LESSEQUAL;
    case Renderer::Compare::Greater: return D3DCMP_GREATER;
    case Renderer::Compare::NotEqual: return D3DCMP_NOTEQUAL;
    case Renderer::Compare::GreaterEqual: return D3DCMP_GREATEREQUAL;
    case Renderer::Compare::Always: return D3DCMP_ALWAYS;
    }
    WWASSERT(0);
    return D3DCMP_NEVER;
}
DWORD To_D3D_BlendFactor(Renderer::BlendFactor value)
{
    switch (value)
    {
    case Renderer::BlendFactor::Zero: return D3DBLEND_ZERO;
    case Renderer::BlendFactor::One: return D3DBLEND_ONE;
    case Renderer::BlendFactor::SrcColor: return D3DBLEND_SRCCOLOR;
    case Renderer::BlendFactor::InvSrcColor: return D3DBLEND_INVSRCCOLOR;
    case Renderer::BlendFactor::SrcAlpha: return D3DBLEND_SRCALPHA;
    case Renderer::BlendFactor::InvSrcAlpha: return D3DBLEND_INVSRCALPHA;
    case Renderer::BlendFactor::DestAlpha: return D3DBLEND_DESTALPHA;
    case Renderer::BlendFactor::InvDestAlpha: return D3DBLEND_INVDESTALPHA;
    case Renderer::BlendFactor::DestColor: return D3DBLEND_DESTCOLOR;
    case Renderer::BlendFactor::InvDestColor: return D3DBLEND_INVDESTCOLOR;
    case Renderer::BlendFactor::SrcAlphaSat: return D3DBLEND_SRCALPHASAT;
    }
    WWASSERT(0);
    return D3DBLEND_ZERO;
}
DWORD To_D3D_Cull(Renderer::Cull value)
{
    switch (value)
    {
    case Renderer::Cull::None: return D3DCULL_NONE;
    case Renderer::Cull::Clockwise: return D3DCULL_CW;
    case Renderer::Cull::CounterClockwise: return D3DCULL_CCW;
    }
    WWASSERT(0);
    return D3DCULL_NONE;
}
DWORD To_D3D_StencilOp(Renderer::StencilOp value)
{
    switch (value)
    {
    case Renderer::StencilOp::Keep: return D3DSTENCILOP_KEEP;
    case Renderer::StencilOp::Zero: return D3DSTENCILOP_ZERO;
    case Renderer::StencilOp::Replace: return D3DSTENCILOP_REPLACE;
    case Renderer::StencilOp::IncrementSaturate: return D3DSTENCILOP_INCRSAT;
    case Renderer::StencilOp::DecrementSaturate: return D3DSTENCILOP_DECRSAT;
    case Renderer::StencilOp::Invert: return D3DSTENCILOP_INVERT;
    case Renderer::StencilOp::IncrementWrap: return D3DSTENCILOP_INCR;
    case Renderer::StencilOp::DecrementWrap: return D3DSTENCILOP_DECR;
    }
    WWASSERT(0);
    return D3DSTENCILOP_KEEP;
}
}
void Renderer::Set_Depth_Compare(Compare value, StateWrite mode)
{
    Write_Render_State(D3DRS_ZFUNC,To_D3D_Compare(value),mode);
}
void Renderer::Set_Depth_Write(bool value, StateWrite mode)
{
    Write_Render_State(D3DRS_ZWRITEENABLE,value ? TRUE : FALSE,mode);
}
void Renderer::Set_Depth_Test(bool value, StateWrite mode)
{
    Write_Render_State(D3DRS_ZENABLE,value ? D3DZB_TRUE : D3DZB_FALSE,mode);
}
void Renderer::Set_Source_Blend(BlendFactor value, StateWrite mode)
{
    Write_Render_State(D3DRS_SRCBLEND,To_D3D_BlendFactor(value),mode);
}
void Renderer::Set_Destination_Blend(BlendFactor value, StateWrite mode)
{
    Write_Render_State(D3DRS_DESTBLEND,To_D3D_BlendFactor(value),mode);
}
void Renderer::Set_Blend_Enable(bool value, StateWrite mode)
{
    Write_Render_State(D3DRS_ALPHABLENDENABLE,value ? TRUE : FALSE,mode);
}
void Renderer::Set_Alpha_Test(bool value, StateWrite mode)
{
    Write_Render_State(D3DRS_ALPHATESTENABLE,value ? TRUE : FALSE,mode);
}
void Renderer::Set_Alpha_Reference(unsigned value, StateWrite mode)
{
    Write_Render_State(D3DRS_ALPHAREF,value,mode);
}
void Renderer::Set_Alpha_Compare(Compare value, StateWrite mode)
{
    Write_Render_State(D3DRS_ALPHAFUNC,To_D3D_Compare(value),mode);
}
void Renderer::Set_Cull(Cull value, StateWrite mode)
{
    Write_Render_State(D3DRS_CULLMODE,To_D3D_Cull(value),mode);
}
void Renderer::Set_Stencil_Enable(bool value, StateWrite mode)
{
    Write_Render_State(D3DRS_STENCILENABLE,value ? TRUE : FALSE,mode);
}
void Renderer::Set_Stencil_Compare(Compare value, StateWrite mode)
{
    Write_Render_State(D3DRS_STENCILFUNC,To_D3D_Compare(value),mode);
}
void Renderer::Set_Stencil_Reference(unsigned value, StateWrite mode)
{
    Write_Render_State(D3DRS_STENCILREF,value,mode);
}
void Renderer::Set_Stencil_Read_Mask(unsigned value, StateWrite mode)
{
    Write_Render_State(D3DRS_STENCILMASK,value,mode);
}
void Renderer::Set_Stencil_Write_Mask(unsigned value, StateWrite mode)
{
    Write_Render_State(D3DRS_STENCILWRITEMASK,value,mode);
}
void Renderer::Set_Stencil_Fail(StencilOp value, StateWrite mode)
{
    Write_Render_State(D3DRS_STENCILFAIL,To_D3D_StencilOp(value),mode);
}
void Renderer::Set_Stencil_Depth_Fail(StencilOp value, StateWrite mode)
{
    Write_Render_State(D3DRS_STENCILZFAIL,To_D3D_StencilOp(value),mode);
}
void Renderer::Set_Stencil_Pass(StencilOp value, StateWrite mode)
{
    Write_Render_State(D3DRS_STENCILPASS,To_D3D_StencilOp(value),mode);
}


namespace
{
void Write_Texture_Stage(unsigned stage,D3DTEXTURESTAGESTATETYPE state,DWORD value,Renderer::TextureStageWrite mode)
{
    if (mode == Renderer::TextureStageWrite::Cached)
        DX8Wrapper::Set_DX8_Texture_Stage_State(stage,state,value);
    else
    {
        DX8CALL(SetTextureStageState(stage,state,value));
    }
}
DWORD To_D3D_TextureOperation(Renderer::TextureOperation value)
{
    switch(value)
    {
    case Renderer::TextureOperation::SelectArg1: return D3DTOP_SELECTARG1;
    case Renderer::TextureOperation::Disable: return D3DTOP_DISABLE;
    case Renderer::TextureOperation::Modulate: return D3DTOP_MODULATE;
    case Renderer::TextureOperation::Add: return D3DTOP_ADD;
    case Renderer::TextureOperation::BumpEnvMap: return D3DTOP_BUMPENVMAP;
    case Renderer::TextureOperation::BumpEnvMapLuminance: return D3DTOP_BUMPENVMAPLUMINANCE;
    case Renderer::TextureOperation::Modulate2X: return D3DTOP_MODULATE2X;
    case Renderer::TextureOperation::SelectArg2: return D3DTOP_SELECTARG2;
    case Renderer::TextureOperation::AddSmooth: return D3DTOP_ADDSMOOTH;
    case Renderer::TextureOperation::Subtract: return D3DTOP_SUBTRACT;
    case Renderer::TextureOperation::BlendTextureAlpha: return D3DTOP_BLENDTEXTUREALPHA;
    case Renderer::TextureOperation::BlendCurrentAlpha: return D3DTOP_BLENDCURRENTALPHA;
    case Renderer::TextureOperation::AddSigned: return D3DTOP_ADDSIGNED;
    case Renderer::TextureOperation::AddSigned2X: return D3DTOP_ADDSIGNED2X;
    case Renderer::TextureOperation::ModulateAlphaAddColor: return D3DTOP_MODULATEALPHA_ADDCOLOR;
    }
    WWASSERT(0);
    return D3DTOP_SELECTARG1;
}
DWORD To_D3D_TextureArgument(Renderer::TextureArgument value)
{
    switch(value)
    {
    case Renderer::TextureArgument::Diffuse: return D3DTA_DIFFUSE;
    case Renderer::TextureArgument::Texture: return D3DTA_TEXTURE;
    case Renderer::TextureArgument::Current: return D3DTA_CURRENT;
    case Renderer::TextureArgument::Factor: return D3DTA_TFACTOR;
    }
    WWASSERT(0);
    return D3DTA_DIFFUSE;
}
}
void Renderer::Set_Color_Operation(unsigned stage, TextureOperation value, TextureStageWrite mode)
{
    Write_Texture_Stage(stage,D3DTSS_COLOROP,To_D3D_TextureOperation(value),mode);
}
void Renderer::Set_Alpha_Operation(unsigned stage, TextureOperation value, TextureStageWrite mode)
{
    Write_Texture_Stage(stage,D3DTSS_ALPHAOP,To_D3D_TextureOperation(value),mode);
}
void Renderer::Set_Color_Argument1(unsigned stage, TextureArgument value, TextureStageWrite mode)
{
    Write_Texture_Stage(stage,D3DTSS_COLORARG1,To_D3D_TextureArgument(value),mode);
}
void Renderer::Set_Color_Argument2(unsigned stage, TextureArgument value, TextureStageWrite mode)
{
    Write_Texture_Stage(stage,D3DTSS_COLORARG2,To_D3D_TextureArgument(value),mode);
}
void Renderer::Set_Alpha_Argument1(unsigned stage, TextureArgument value, TextureStageWrite mode)
{
    Write_Texture_Stage(stage,D3DTSS_ALPHAARG1,To_D3D_TextureArgument(value),mode);
}
void Renderer::Set_Alpha_Argument2(unsigned stage, TextureArgument value, TextureStageWrite mode)
{
    Write_Texture_Stage(stage,D3DTSS_ALPHAARG2,To_D3D_TextureArgument(value),mode);
}
void Renderer::Set_Untracked_Passthrough_Coordinates(unsigned stage,unsigned coordinate_index)
{
    DX8CALL(SetTextureStageState(stage,D3DTSS_TEXCOORDINDEX,D3DTSS_TCI_PASSTHRU | coordinate_index));
}
void Renderer::Clear_Untracked_Texture(unsigned stage)
{
    DX8CALL(SetTexture(stage,nullptr));
}

void Renderer::Draw_Triangles(DrawRoute route, unsigned short start_index, unsigned short polygon_count,
                              unsigned short min_vertex_index, unsigned short vertex_count)
{
    unsigned type;
    switch (route)
    {
    case DrawRoute::Gpu: type = BUFFER_TYPE_DX8; break;
    case DrawRoute::CpuSorting: type = BUFFER_TYPE_SORTING; break;
    case DrawRoute::DynamicGpu: type = BUFFER_TYPE_DYNAMIC_DX8; break;
    case DrawRoute::DynamicCpuSorting: type = BUFFER_TYPE_DYNAMIC_SORTING; break;
    default: WWASSERT(0); return;
    }
    DX8Wrapper::Draw_Triangles(type,start_index,polygon_count,min_vertex_index,vertex_count);
}

void Renderer::Draw_Strip(unsigned short start_index, unsigned short polygon_count,
                          unsigned short min_vertex_index, unsigned short vertex_count)
{
    DX8Wrapper::Draw_Strip(start_index,polygon_count,min_vertex_index,vertex_count);
}
