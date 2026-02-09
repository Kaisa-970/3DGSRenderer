#include "Renderable.h"

RENDERER_NAMESPACE_BEGIN

Renderable::Renderable()
    : m_uid(0), m_type(RenderableType::Primitive), m_transformMatrix(Matrix4::identity()), m_material(nullptr),
      m_primitive(nullptr), m_model(nullptr), m_color(Vector3(1.0f, 1.0f, 1.0f))
{
}

Renderable::Renderable(unsigned int uid, const Matrix4 &transformMatrix, const std::shared_ptr<Material> &material,
                       const std::shared_ptr<Primitive> &primitive)
    : m_uid(uid), m_type(RenderableType::Primitive), m_transformMatrix(transformMatrix), m_material(material),
      m_primitive(primitive), m_model(nullptr), m_color(Vector3(1.0f, 1.0f, 1.0f))
{
}

Renderable::Renderable(unsigned int uid, const Matrix4 &transformMatrix, const std::shared_ptr<Model> &model)
    : m_uid(uid), m_type(RenderableType::Model), m_transformMatrix(transformMatrix), m_material(nullptr),
      m_primitive(nullptr), m_model(model), m_color(Vector3(1.0f, 1.0f, 1.0f))
{
}

Renderable::~Renderable()
{
}

Renderable::Renderable(Renderable &&other) noexcept
    : m_uid(other.m_uid), m_type(other.m_type), m_transformMatrix(other.m_transformMatrix),
      m_material(other.m_material), m_primitive(other.m_primitive), m_model(other.m_model), m_color(other.m_color)
{
    other.m_uid = 0;
    other.m_type = RenderableType::Primitive;
    other.m_transformMatrix = Matrix4::identity();
    other.m_material = nullptr;
    other.m_primitive = nullptr;
    other.m_model = nullptr;
    other.m_color = Vector3(1.0f, 1.0f, 1.0f);
}

Renderable &Renderable::operator=(Renderable &&other) noexcept
{
    if (this != &other)
    {
        m_uid = other.m_uid;
        m_type = other.m_type;
        m_transformMatrix = other.m_transformMatrix;
        m_material = other.m_material;
        m_primitive = other.m_primitive;
        m_model = other.m_model;
        m_color = other.m_color;
        other.m_uid = 0;
        other.m_type = RenderableType::Primitive;
        other.m_transformMatrix = Matrix4::identity();
        other.m_material = nullptr;
        other.m_primitive = nullptr;
        other.m_model = nullptr;
        other.m_color = Vector3(1.0f, 1.0f, 1.0f);
    }
    return *this;
}

void Renderable::draw(const std::shared_ptr<Shader> &shader) const
{
    if (m_type == RenderableType::Primitive && m_primitive)
    {
        shader->setMat4("model", m_transformMatrix.m);
        shader->setInt("uUID", static_cast<int>(m_uid));
        shader->setVec3("uColor", m_color.x, m_color.y, m_color.z);
        if (m_material)
        {
            m_material->UpdateShaderParams(shader);
        }
        m_primitive->draw(shader);
    }
    else if (m_type == RenderableType::Model && m_model)
    {
        shader->setMat4("model", m_transformMatrix.m);
        shader->setInt("uUID", static_cast<int>(m_uid));
        shader->setVec3("uColor", m_color.x, m_color.y, m_color.z);
        m_model->draw(shader);
    }
}

RENDERER_NAMESPACE_END
