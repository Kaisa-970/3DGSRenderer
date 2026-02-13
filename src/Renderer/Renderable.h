#pragma once

#include "Core/RenderCore.h"
#include "MathUtils/Matrix.h"
#include "MathUtils/Vector.h"
#include "Material.h"
#include "Primitives/Primitive.h"
#include "Model.h"
#include <memory>
#include "Transform.h"

RENDERER_NAMESPACE_BEGIN

enum class RenderableType
{
    Primitive,
    Model
};

/// Renderable: 纯数据容器
/// 描述"场景中有什么"——几何体/模型 + 材质 + 变换 + 颜色
/// 不包含任何渲染逻辑（"怎么画"由 RenderPass 负责）
class RENDERER_API Renderable
{
public:
    Renderable();
    Renderable(unsigned int uid, const Mat4 &transformMatrix, const std::shared_ptr<Material> &material,
               const std::shared_ptr<Primitive> &primitive);
    Renderable(unsigned int uid, const Mat4 &transformMatrix, const std::shared_ptr<Model> &model);
    ~Renderable();
    Renderable(const Renderable &other) = delete;
    Renderable &operator=(const Renderable &other) = delete;
    Renderable(Renderable &&other) noexcept;
    Renderable &operator=(Renderable &&other) noexcept;

    // ---- 访问器 ----
    const std::string &getName() const
    {
        return m_name;
    }
    void setName(const std::string &name)
    {
        m_name = name;
    }
    unsigned int getUid() const
    {
        return m_uid;
    }
    void setUid(unsigned int newUid)
    {
        m_uid = newUid;
    }

    const Mat4 &getTransform() const
    {
        return m_transformMatrix;
        // return m_transform.GetMatrix();
    }
    void setTransform(const Mat4 &m)
    {
        m_transformMatrix = m;
    }

    const std::shared_ptr<Material> &getMaterial() const
    {
        return m_material;
    }
    void setMaterial(const std::shared_ptr<Material> &mat)
    {
        m_material = mat;
    }

    const std::shared_ptr<Primitive> &getPrimitive() const
    {
        return m_primitive;
    }
    void setPrimitive(const std::shared_ptr<Primitive> &prim)
    {
        m_primitive = prim;
        m_type = RenderableType::Primitive;
        m_model = nullptr;
    }

    const std::shared_ptr<Model> &getModel() const
    {
        return m_model;
    }
    void setModel(const std::shared_ptr<Model> &mdl)
    {
        m_model = mdl;
        m_type = RenderableType::Model;
        m_primitive = nullptr;
    }

    RenderableType getType() const
    {
        return m_type;
    }

    const Vector3 &getColor() const
    {
        return m_color;
    }
    void setColor(const Vector3 &c)
    {
        m_color = c;
    }

public:
    Transform m_transform;

private:
    std::string m_name;
    RenderableType m_type;
    Mat4 m_transformMatrix;
    Vector3 m_color;
    unsigned int m_uid;
    std::shared_ptr<Material> m_material;
    std::shared_ptr<Primitive> m_primitive;
    std::shared_ptr<Model> m_model;
};

RENDERER_NAMESPACE_END
