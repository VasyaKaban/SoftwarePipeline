#include "RenderableMesh.h"
#include <cstring>
#include <stdexcept>


RenderableMesh::RenderableMesh(RenderableMesh &&rm) noexcept
	: vertex_data(std::move(rm.vertex_data)),
	  index_data(std::move(rm.index_data)),
	  parts(std::move(rm.parts)) {}

RenderableMesh & RenderableMesh::operator=(RenderableMesh &&rm) noexcept
{
	this->~RenderableMesh();

	vertex_data = std::move(rm.vertex_data);
	index_data = std::move(rm.index_data);
	parts = std::move(rm.parts);

	return *this;
}

void RenderableMesh::Create(const MeshVertexIndexData &data/*,
							const std::map<MaterialTreeKey, std::unique_ptr<Material>> &materials*/)
{
	std::vector<std::byte> _vertex_data(data.vertex_attributes.size() * sizeof(MeshVertexAttribute));
	std::memcpy(_vertex_data.data(), data.vertex_attributes.data(), _vertex_data.size());

	std::vector<RenderablePart> _parts;
	_parts.reserve(data.part_indices.size());


	std::size_t common_indices_size = 0;
	for(const auto &ind : data.part_indices)
		common_indices_size += ind.indices.size();

	std::vector<std::uint32_t> _index_data(common_indices_size);

	std::size_t offset = 0;
	for(const auto &ind : data.part_indices)
	{
		_parts.push_back(RenderablePart{.count = ind.indices.size(),
										.offset = offset/*,
										.material = materials.find(MaterialTreeKey(ind.material_lib_name, ind.material_name))->second.get()*/});

		std::memcpy(_index_data.data() + offset, ind.indices.data(), ind.indices.size() * 4);
		offset += ind.indices.size();
	}

	vertex_data = std::move(_vertex_data);
	index_data = std::move(_index_data);
	parts = std::move(_parts);
}

const std::vector<RenderablePart> & RenderableMesh::GetParts() const noexcept
{
	return parts;
}

const std::vector<std::byte> & RenderableMesh::GetVertexData() const noexcept
{
	return vertex_data;
}
const std::vector<std::uint32_t> & RenderableMesh::GetIndexData() const noexcept
{
	return index_data;
}
