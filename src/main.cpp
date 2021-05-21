#include "tiny_obj_loader.h"

using Faces = std::vector<std::size_t>;
using Edge  = std::pair<std::size_t, std::size_t>;
using Edges = std::vector<Edge>;

/* Get vertex id list such that every 3 consequent ids from the beginning
 * form a face.
 */
Faces get_face_verts(std::vector<tinyobj::shape_t> const &shapes) {
    std::vector<std::size_t> faces;
    for (auto const &shape : shapes) {
        std::size_t index_offset = 0;
        for (std::size_t fi = 0; fi < shape.mesh.num_face_vertices.size();
             ++fi) {
            std::size_t nverts = shape.mesh.num_face_vertices[fi];
            for (std::size_t v = 0; v < nverts; ++v) {
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
                faces.push_back(idx.vertex_index);
            }
        }
    }
    return faces;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <model.obj>\n", argv[0]);
        return 1;
    }
    tinyobj::ObjReader       reader;
    tinyobj::ObjReaderConfig config;
    config.triangulate = true;
    if (!reader.ParseFromFile(argv[1], config)) {
        if (!reader.Error().empty()) {
            printf("tinyobj: %s\n", reader.Error().c_str());
        }
        return 2;
    }

    Faces faces = get_face_verts(reader.GetShapes());
    printf("%lu\n", faces.size());

    return 0;
}

// Author: Blurgy <gy@blurgy.xyz>
// Date:   May 21 2021, 10:49 [CST]
