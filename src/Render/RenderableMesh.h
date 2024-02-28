#pragma once

#include "../Wavefront/Mesh.h"
//#include "../Material/Material.h"
#include <vector>
#include <map>

struct RenderablePart
{
	std::size_t count;
	std::size_t offset;
	//const Material *material;
};

class RenderableMesh
{
public:
	RenderableMesh() = default;
	~RenderableMesh() = default;
	RenderableMesh(const RenderableMesh &) = delete;
	RenderableMesh(RenderableMesh &&rm) noexcept;
	RenderableMesh & operator=(const RenderableMesh &) = delete;
	RenderableMesh & operator=(RenderableMesh &&rm) noexcept;

	void Create(const MeshVertexIndexData &data/*, const std::map<MaterialTreeKey, std::unique_ptr<Material>> &materials*/);

	const std::vector<RenderablePart> & GetParts() const noexcept;

	const std::vector<std::byte> & GetVertexData() const noexcept;
	const std::vector<std::uint32_t> & GetIndexData() const noexcept;

private:
	std::vector<std::byte> vertex_data;
	std::vector<std::uint32_t> index_data;
	std::vector<RenderablePart> parts;
};
