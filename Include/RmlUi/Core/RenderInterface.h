/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef RMLUI_CORE_RENDERINTERFACE_H
#define RMLUI_CORE_RENDERINTERFACE_H

#include "Header.h"
#include "Texture.h"
#include "Traits.h"
#include "Types.h"
#include "Vertex.h"

namespace Rml {

class Context;

enum class ClipMaskOperation { Clip, ClipIntersect, ClipOut };
enum class RenderClear { None, Clear, Clone };
enum class RenderTarget { Layer, MaskImage, RenderTexture };
enum class BlendMode { Blend, Replace };

using RenderCommandUserData = uintptr_t;
using FilterHandleList = Vector<CompiledFilterHandle>;

struct RenderCommand {
	enum class Type {
		RenderGeometry,

		EnableClipMask,
		DisableClipMask,
		RenderClipMask,

		PushLayer,
		PopLayer,

		RenderShader,

		AttachFilter,
	};
	Type type;

	// -- Geometry, RenderClipMask, RenderShader
	int vertices_offset;
	int indices_offset;
	int num_elements;

	int translation_offset;
	int transform_offset;

	int scissor_offset;

	TextureHandle texture; // Texture to attach to the geometry. PopLayer: Render texture target.

	// -- RenderClipMask
	ClipMaskOperation clip_mask_operation;

	// -- RenderShader
	CompiledShaderHandle shader;

	// -- PushLayer
	RenderClear clear_new_layer;

	// -- PopLayer
	RenderTarget render_target;
	BlendMode blend_mode;
	int filter_lists_offset;

	// -- All
	RenderCommandUserData user_data;
};

struct RenderCommandList {
	Vector<Vertex> vertices;
	Vector<int> indices;

	Vector<Vector2f> translations;
	Vector<Matrix4f> transforms;

	Vector<Rectanglei> scissor_regions;

	Vector<FilterHandleList> filter_lists;

	Vector<RenderCommand> commands;
};

/**
    The abstract base class for application-specific rendering implementation. Your application must provide a concrete
    implementation of this class and install it through Rml::SetRenderInterface() in order for anything to be rendered.

    @author Peter Curry
 */

class RMLUICORE_API RenderInterface : public NonCopyMoveable {
public:
	RenderInterface();
	virtual ~RenderInterface();

	/// Called by RmlUi when a texture is required by the library.
	/// @param[out] texture_handle The handle to write the texture handle for the loaded texture to.
	/// @param[out] texture_dimensions The variable to write the dimensions of the loaded texture.
	/// @param[in] source The application-defined image source, joined with the path of the referencing document.
	/// @return True if the load attempt succeeded and the handle and dimensions are valid, false if not.
	virtual bool LoadTexture(TextureHandle& texture_handle, Vector2i& texture_dimensions, const String& source);
	/// Called by RmlUi when a texture is required to be built from an internally-generated sequence of pixels.
	/// @param[out] texture_handle The handle to write the texture handle for the generated texture to.
	/// @param[in] source The raw 8-bit texture data. Each pixel is made up of four 8-bit values, indicating red, green, blue and alpha in that order.
	/// @param[in] source_dimensions The dimensions, in pixels, of the source data.
	/// @return True if the texture generation succeeded and the handle is valid, false if not.
	virtual bool GenerateTexture(TextureHandle& texture_handle, const byte* source, const Vector2i& source_dimensions);
	/// Called by RmlUi when it wants to create a render texture it can use as a render target, and subsequently render as a normal texture.
	virtual bool GenerateRenderTexture(TextureHandle& texture_handle, Vector2i dimensions);
	/// Called by RmlUi when a loaded texture or render texture is no longer required.
	/// @param texture The texture handle to release.
	virtual void ReleaseTexture(TextureHandle texture);

	/// Called by RmlUi when...
	virtual CompiledShaderHandle CompileShader(const String& name, const Dictionary& parameters);
	/// Called by RmlUi when...
	virtual void ReleaseCompiledShader(CompiledShaderHandle shader);

	/// Called by RmlUi when...
	virtual CompiledFilterHandle CompileFilter(const String& name, const Dictionary& parameters);
	/// Called by RmlUi when...
	virtual void ReleaseCompiledFilter(CompiledFilterHandle filter);

	/// Get the context currently being rendered. This is only valid during RenderGeometry,
	/// CompileGeometry, RenderCompiledGeometry, EnableScissorRegion and SetScissorRegion.
	Context* GetContext() const;

	// -- DEPRECATED API (do nothing) --

	/// Called by RmlUi when it wants to render geometry that the application does not wish to optimise. Note that
	/// RmlUi renders everything as triangles.
	/// @param[in] vertices The geometry's vertex data.
	/// @param[in] num_vertices The number of vertices passed to the function.
	/// @param[in] indices The geometry's index data.
	/// @param[in] num_indices The number of indices passed to the function. This will always be a multiple of three.
	/// @param[in] texture The texture to be applied to the geometry. This may be nullptr, in which case the geometry is untextured.
	/// @param[in] translation The translation to apply to the geometry.
	/// @note Affected by transform: Yes. Affected by scissor: Yes. Affected by clip mask: Yes.
	void RenderGeometry(Vertex* /*vertices*/, int /*num_vertices*/, int* /*indices*/, int /*num_indices*/, TextureHandle /*texture*/,
		const Vector2f& /*translation*/)
	{}

	/// Called by RmlUi when it wants to compile geometry it believes will be static for the forseeable future.
	/// If supported, this should return a handle to an optimised, application-specific version of the data. If
	/// not, do not override the function or return zero; the simpler RenderGeometry() will be called instead.
	/// @param[in] vertices The geometry's vertex data.
	/// @param[in] num_vertices The number of vertices passed to the function.
	/// @param[in] indices The geometry's index data.
	/// @param[in] num_indices The number of indices passed to the function. This will always be a multiple of three.
	/// @param[in] texture The texture to be applied to the geometry. This may be nullptr, in which case the geometry is untextured.
	/// @return The application-specific compiled geometry. Compiled geometry will be stored and rendered using RenderCompiledGeometry() in future
	/// calls, and released with ReleaseCompiledGeometry() when it is no longer needed.
	CompiledGeometryHandle CompileGeometry(Vertex* vertices, int num_vertices, int* indices, int num_indices, TextureHandle texture);
	/// Called by RmlUi when it wants to render application-compiled geometry.
	/// @param[in] geometry The application-specific compiled geometry to render.
	/// @param[in] translation The translation to apply to the geometry.
	/// @note Affected by transform: Yes. Affected by scissor: Yes. Affected by clip mask: Yes.
	void RenderCompiledGeometry(CompiledGeometryHandle geometry, const Vector2f& translation);
	/// Called by RmlUi when it wants to release application-compiled geometry.
	/// @param[in] geometry The application-specific compiled geometry to release.
	void ReleaseCompiledGeometry(CompiledGeometryHandle geometry);

	/// Called by RmlUi when it wants to enable or disable scissoring to clip content.
	/// @param[in] enable True if scissoring is to enabled, false if it is to be disabled.
	void EnableScissorRegion(bool /*enable*/) {}
	/// Called by RmlUi when it wants to change the scissor region.
	/// @param[in] x The left-most pixel to be rendered. All pixels to the left of this should be clipped.
	/// @param[in] y The top-most pixel to be rendered. All pixels to the top of this should be clipped.
	/// @param[in] width The width of the scissored region. All pixels to the right of (x + width) should be clipped.
	/// @param[in] height The height of the scissored region. All pixels to below (y + height) should be clipped.
	/// @note Affected by transform: No. Affected by scissor: No. Affected by clip mask: No.
	void SetScissorRegion(int /*x*/, int /*y*/, int /*width*/, int /*height*/) {}

	bool EnableClipMask(bool enable);
	/// @note Affected by transform: Yes. Affected by scissor: Yes. Affected by clip mask: See arguments.
	void RenderToClipMask(ClipMaskOperation mask_operation, CompiledGeometryHandle geometry, Vector2f translation);

	/// Called by RmlUi when it wants the renderer to use a new transform matrix.
	/// This will only be called if 'transform' properties are encountered. If no transform applies to the current element, nullptr
	/// is submitted. Then it expects the renderer to use an identity matrix or otherwise omit the multiplication with the transform.
	/// @param[in] transform The new transform to apply, or nullptr if no transform applies to the current element.
	void SetTransform(const Matrix4f* transform);

	/// Called by RmlUi when...
	/// @note Affected by transform: No. Affected by scissor: Yes. Affected by clip mask: Yes.
	void PushLayer(RenderClear clear_new_layer);
	/// Called by RmlUi when...
	/// @return A handle to the resulting render texture, or zero if the render target is not a render texture.
	/// @note Should render the current layer to the target specified using the given blend mode.
	/// @note Should apply attached filters and mask image, and then clear these attachments.
	/// @note Render texture targets should be dimensioned and extracted from the bounds of the active scissor.
	/// @note Affected by transform: No. Affected by scissor: Yes. Affected by clip mask: Yes.
	TextureHandle PopLayer(RenderTarget render_target, BlendMode blend_mode);

	/// Render geometry with the given shader.
	void RenderShader(CompiledShaderHandle shader, CompiledGeometryHandle geometry, Vector2f translation);

	/// Attach filter to be applied on the next call to PopLayer.
	void AttachFilter(CompiledFilterHandle filter);

private:
	Context* context;

	friend class Rml::Context;
};

} // namespace Rml
#endif
