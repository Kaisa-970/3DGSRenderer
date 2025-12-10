#pragma once

#include "Core/RenderCore.h"
#include "Shader.h"
#include "MathUtils/Matrix.h"
#include "Material.h"
#include "Primitives/Primitive.h"
#include "Model.h"
#include <memory>

RENDERER_NAMESPACE_BEGIN

enum class RenderableType
{
    Primitive,
    Model
};

class RENDERER_API Renderable {
public:
    Renderable();
    Renderable(unsigned int uid, const Matrix4& transformMatrix, const std::shared_ptr<Material>& material, const std::shared_ptr<Primitive>& primitive);
    Renderable(unsigned int uid, const Matrix4& transformMatrix, const std::shared_ptr<Model>& model);
    virtual ~Renderable();
    Renderable(const Renderable& other) = delete;
    Renderable& operator=(const Renderable& other) = delete;
    Renderable(Renderable&& other) noexcept;
    Renderable& operator=(Renderable&& other) noexcept;

    virtual void draw(const Shader& shader) const;

    // 访问/修改
    unsigned int getUid() const { return m_uid; }
    void setUid(unsigned int newUid) { m_uid = newUid; }

    const Matrix4& getTransform() const { return m_transformMatrix; }
    void setTransform(const Matrix4& m) { m_transformMatrix = m; }

    const std::shared_ptr<Material>& getMaterial() const { return m_material; }
    void setMaterial(const std::shared_ptr<Material>& mat) { m_material = mat; }

    const std::shared_ptr<Primitive>& getPrimitive() const { return m_primitive; }
    void setPrimitive(const std::shared_ptr<Primitive>& prim) { m_primitive = prim; m_type = RenderableType::Primitive; m_model = nullptr; }

    const std::shared_ptr<Model>& getModel() const { return m_model; }
    void setModel(const std::shared_ptr<Model>& mdl) { m_model = mdl; m_type = RenderableType::Model; m_primitive = nullptr; }

    RenderableType getType() const { return m_type; }

    const Vector3& getColor() const { return m_color; }
    void setColor(const Vector3& c) { m_color = c; }

protected:
    unsigned int m_uid;
    RenderableType m_type;
    Matrix4 m_transformMatrix;
    std::shared_ptr<Material> m_material;
    std::shared_ptr<Primitive> m_primitive;
    std::shared_ptr<Model> m_model;
    Vector3 m_color;
};

RENDERER_NAMESPACE_END