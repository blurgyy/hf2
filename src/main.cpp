#include "tiny_obj_loader.h"

#include <list>
#include <map>
#include <set>

using Faces = std::vector<std::size_t>;
using Edge  = std::pair<std::size_t, std::size_t>;
using Edges = std::set<Edge>;

/* Any border vertex is referenced by at most 2 border edges. */
using BorderVertices = std::map<std::size_t, std::pair<Edge, Edge>>;

using ConnectedEdges = std::list<Edge>;

void flip(Edge &edge) { edge = {edge.second, edge.first}; }

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
                faces.push_back(nverts * idx.vertex_index);
            }
            index_offset += nverts;
        }
    }
    return faces;
}

Edges get_border_edges(Faces const faces) {
    Edges border_edges;

    for (std::size_t i = 0; i < faces.size(); i += 3) {
        std::size_t v0 = faces[i + 0];
        std::size_t v1 = faces[i + 1];
        std::size_t v2 = faces[i + 2];

        Edge e0 = {v0, v1}, e0_i = {v1, v0};
        Edge e1 = {v1, v2}, e1_i = {v2, v1};
        Edge e2 = {v2, v0}, e2_i = {v0, v2};

        if (border_edges.find(e0_i) == border_edges.end()) {
            // printf("inserted: (%lu, %lu)\n", e0.first, e0.second);
            border_edges.insert(e0);
        } else {
            border_edges.erase(e0_i);
        }

        if (border_edges.find(e1_i) == border_edges.end()) {
            // printf("inserted: (%lu, %lu)\n", e1.first, e1.second);
            border_edges.insert(e1);
        } else {
            border_edges.erase(e1_i);
        }

        if (border_edges.find(e2_i) == border_edges.end()) {
            // printf("inserted: (%lu, %lu)\n", e2.first, e2.second);
            border_edges.insert(e2);
        } else {
            border_edges.erase(e2_i);
        }
    }

    for (auto e : border_edges) {
        printf("%lu - %lu\n", e.first, e.second);
    }

    return border_edges;
}

/* Pass as reference to allow removal of recorded edges. */
ConnectedEdges get_connected_border(Edges &border_edges) {
    ConnectedEdges connected_border;

    BorderVertices border_vertices;
    for (Edge const &edge : border_edges) {
        std::size_t v0             = edge.first;
        std::size_t v1             = edge.second;
        border_vertices[v0].first  = edge;
        border_vertices[v1].second = edge;
    }

    connected_border.push_back(*border_edges.begin());
    Edge current_edge = connected_border.back();
    /* Remove edges that has been recorded */
    border_edges.erase(current_edge);
    Edge next_edge = border_vertices[current_edge.second].first;
    while (next_edge != connected_border.front()) {
        connected_border.push_back(next_edge);
        current_edge = connected_border.back();
        /* Remove edges that has been recorded */
        border_edges.erase(current_edge);
        next_edge = border_vertices[current_edge.second].first;
    }

    /* Reverse border to get correct ordering of border edges. */
    std::reverse(connected_border.begin(), connected_border.end());
    std::for_each(connected_border.begin(), connected_border.end(), flip);

    return connected_border;
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
    Edges border_edges = get_border_edges(faces);
    printf("%lu\n", border_edges.size());

    std::vector<ConnectedEdges> connected_border_edges;
    while (border_edges.size()) {
        printf("Border edges has size %lu\n", border_edges.size());
        connected_border_edges.push_back(get_connected_border(border_edges));
    }
    printf("Found %lu borders\n", connected_border_edges.size());

    return 0;
}

// Author: Blurgy <gy@blurgy.xyz>
// Date:   May 21 2021, 10:49 [CST]
