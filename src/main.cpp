#include "tiny_obj_loader.h"

#include <glm/glm.hpp>

#include <fmt/core.h>

#include <filesystem>
#include <fstream>
#include <list>
#include <map>
#include <set>
#include <stdexcept>

namespace fs = std::filesystem;

using fmt::format;

using flt  = double;
using vec3 = glm::vec<3, flt, glm::defaultp>;

using Faces = std::vector<std::size_t>;
using Edge  = std::pair<std::size_t, std::size_t>;
using Edges = std::set<Edge>;

using Vertices = std::vector<vec3>;
using Normals  = std::vector<vec3>;

/* Map vertex index to normal index (for tiny_obj_loader) */
using VertexNormalMapping = std::map<std::size_t, std::size_t>;

/* Map an edge to a face index */
using EdgeFaceMapping = std::map<Edge, std::size_t>;

/* Any border vertex is referenced by at most 2 border edges. */
using BorderVertices = std::map<std::size_t, std::pair<Edge, Edge>>;
using ConnectedEdges = std::list<Edge>;

void flip(Edge &edge) { edge = {edge.second, edge.first}; }

/* Get vertex id list such that every 3 consequent ids from the beginning
 * form a face.
 */
Faces get_all_faces(tinyobj::shape_t const &shape,
                    VertexNormalMapping &   normal_index_of) {
    Faces       faces;
    std::size_t index_offset = 0;
    for (std::size_t fi = 0; fi < shape.mesh.num_face_vertices.size(); ++fi) {
        std::size_t nverts = shape.mesh.num_face_vertices[fi];
        for (std::size_t v = 0; v < nverts; ++v) {
            tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
            faces.push_back(idx.vertex_index);
            normal_index_of[idx.vertex_index] = idx.normal_index;
        }
        index_offset += nverts;
    }
    return faces;
}

Vertices get_all_vertices(tinyobj::attrib_t const &attrib) {
    /* Must be triangular mesh */
    Vertices vertices(attrib.vertices.size() / 3);
    auto     get_vertex_from_attrib = [&](std::size_t index) -> vec3 {
        flt x = attrib.vertices[3 * index + 0];
        flt y = attrib.vertices[3 * index + 1];
        flt z = attrib.vertices[3 * index + 2];
        return {x, y, z};
    };
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        vertices[i] = get_vertex_from_attrib(i);
    }
    return vertices;
}

Normals get_all_normals(tinyobj::attrib_t const &attrib,
                        VertexNormalMapping      normal_index_of) {
    /* Must be triangular mesh */
    Normals normals(attrib.normals.size() / 3);
    auto    get_normal_from_attrib = [&](std::size_t index) -> vec3 {
        flt x = attrib.normals[3 * normal_index_of[index] + 0];
        flt y = attrib.normals[3 * normal_index_of[index] + 1];
        flt z = attrib.normals[3 * normal_index_of[index] + 2];
        return {x, y, z};
    };
    for (std::size_t i = 0; i < normals.size(); ++i) {
        normals[i] = get_normal_from_attrib(i);
    }
    return normals;
}

Edges get_border_edges(Faces const &faces, EdgeFaceMapping &face_index_of) {
    Edges border_edges;

    for (std::size_t i = 0; i < faces.size(); i += 3) {
        std::size_t v0 = faces[i + 0];
        std::size_t v1 = faces[i + 1];
        std::size_t v2 = faces[i + 2];

        Edge e0 = {v0, v1}, e0_i = {v1, v0};
        Edge e1 = {v1, v2}, e1_i = {v2, v1};
        Edge e2 = {v2, v0}, e2_i = {v0, v2};
        face_index_of[e0] = face_index_of[e1] = face_index_of[e2] = i / 3;
        face_index_of[e0_i] = face_index_of[e1_i] = face_index_of[e2_i] =
            i / 3;

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

    // for (auto e : border_edges) {
    // printf("%lu - %lu\n", e.first, e.second);
    // }

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
    // printf("Pushing: %lu -> %lu\n", current_edge.first,
    // current_edge.second); scanf("%d");
    /* Remove edges that has been recorded */
    border_edges.erase(current_edge);
    Edge next_edge = border_vertices[current_edge.second].first;
    while (next_edge != connected_border.front()) {
        /* If next edge is invalid or not on border, return empty list */
        if (border_edges.find(next_edge) == border_edges.end()) {
            return ConnectedEdges();
        }
        connected_border.push_back(next_edge);
        current_edge = connected_border.back();
        // printf("Pushing: %lu -> %lu\n", current_edge.first,
        // current_edge.second);
        // scanf("%d");
        /* Remove edges that has been recorded */
        border_edges.erase(current_edge);
        next_edge = border_vertices[current_edge.second].first;
    }

    /* Reverse border to get correct ordering of border edges. */
    std::reverse(connected_border.begin(), connected_border.end());
    std::for_each(connected_border.begin(), connected_border.end(), flip);

    // for (Edge edge : connected_border) {
    // printf("Border edge: %lu -> %lu\n", edge.first, edge.second);
    // }

    return connected_border;
}

Faces close_hole(ConnectedEdges border_edges, tinyobj::shape_t shape,
                 tinyobj::attrib_t const &attrib,
                 VertexNormalMapping      normal_index_of,
                 EdgeFaceMapping          face_index_of) {
    auto get_vertex_from_attrib = [&](std::size_t index) -> vec3 {
        flt x = attrib.vertices[3 * index + 0];
        flt y = attrib.vertices[3 * index + 1];
        flt z = attrib.vertices[3 * index + 2];
        return {x, y, z};
    };
    auto get_normal_of_edge = [&](Edge edge) -> vec3 {
        std::size_t face_index   = face_index_of[edge];
        std::size_t index_offset = 3 * face_index;
        vec3        verts[3];
        for (int i = 0; i < 3; ++i) {
            tinyobj::index_t idx = shape.mesh.indices[index_offset + i];
            verts[i]             = get_vertex_from_attrib(idx.vertex_index);
        }
        vec3 e0 = verts[1] - verts[0];
        vec3 e1 = verts[2] - verts[1];
        return glm::normalize(glm::cross(e0, e1));
    };
    // auto get_normal_from_attrib = [&](std::size_t index) -> vec3 {
    // flt x = attrib.normals[3 * normal_index_of[index] + 0];
    // flt y = attrib.normals[3 * normal_index_of[index] + 1];
    // flt z = attrib.normals[3 * normal_index_of[index] + 2];
    // return {x, y, z};
    // };
    vec3 ref_normal = get_normal_of_edge(border_edges.front());
    /* First, check if normal values are the same across the whole border. */
    for (Edge edge : border_edges) {
        vec3 normal = get_normal_of_edge(edge);
        if (glm::sign(glm::dot(ref_normal, normal)) < 0) {
            throw std::invalid_argument("Sanity check failed: Border with "
                                        "ununified normal is not handled");
        }
    }
    printf("Sanity check for normal directions passed\n");

    Faces added_faces;

    while (border_edges.size() >= 3) {
        for (auto prv_iter = std::prev(border_edges.end()),
                  now_iter = border_edges.begin();
             border_edges.size() >= 3 && now_iter != border_edges.end();
             prv_iter = now_iter, now_iter = std::next(now_iter)) {

            Edge prv = *prv_iter;
            Edge now = *now_iter;

            if (prv.second != now.first) {
                throw std::logic_error("Border edges are not connected");
            }
            std::size_t vid0 = prv.first;
            std::size_t vid1 = prv.second;
            std::size_t vid2 = now.second;

            vec3 v0 = get_vertex_from_attrib(vid0);
            vec3 v1 = get_vertex_from_attrib(vid1);
            vec3 v2 = get_vertex_from_attrib(vid2);

            vec3 e0 = v1 - v0;
            vec3 e1 = v2 - v1;
            if (glm::sign(glm::dot(glm::cross(e0, e1), ref_normal)) < 0) {
                continue;
            }

            added_faces.push_back(vid0);
            added_faces.push_back(vid1);
            added_faces.push_back(vid2);

            auto nxt_iter = std::next(now_iter);
            border_edges.erase(prv_iter);
            border_edges.erase(now_iter);

            Edge new_edge = {vid0, vid2};
            now_iter      = border_edges.insert(nxt_iter, new_edge);
        }
    }

    return added_faces;
}

void export_mesh(fs::path path, Vertices vertices, Normals normals,
                 std::vector<Faces> groups_of_faces) {
    std::ofstream of;
    of.open(path);
    if (of.fail()) {
        throw std::runtime_error(
            "Failed opening output file for added faces");
    }
    of << "\n";
    of << format("# {} vertices\n", vertices.size());
    for (vec3 vert : vertices) {
        of << format("v {} {} {}\n", vert.x, vert.y, vert.z);
    }
    of << "\n";
    of << format("# {} normals\n", vertices.size());
    for (vec3 normal : normals) {
        of << format("vn {} {} {}\n", normal.x, normal.y, normal.z);
    }
    of << "\n";
    for (Faces faces : groups_of_faces) {
        of << format("# Group of {} faces\n", faces.size());
        for (std::size_t i = 0; i < faces.size(); i += 3) {
            of << format("f {} {} {}\n", faces[i + 0] + 1, faces[i + 1] + 1,
                         faces[i + 2] + 1);
        }
    }
    of.close();
}

void export_added_faces(fs::path path, Faces added_faces) {
    std::ofstream of;
    of.open(path);
    if (of.fail()) {
        throw std::runtime_error(
            "Failed opening output file for added faces");
    }
    for (std::size_t i = 0; i < added_faces.size(); i += 3) {
        of << format("f {} {} {}\n", added_faces[i + 0] + 1,
                     added_faces[i + 1] + 1, added_faces[i + 2] + 1);
    }
    of.close();
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

    VertexNormalMapping normal_index_of;
    Faces faces = get_all_faces(reader.GetShapes()[0], normal_index_of);

    Vertices vertices = get_all_vertices(reader.GetAttrib());
    Normals  normals  = get_all_normals(reader.GetAttrib(), normal_index_of);

    EdgeFaceMapping face_index_of;
    Edges           border_edges = get_border_edges(faces, face_index_of);
    printf("%lu edges on the border\n", border_edges.size());

    std::vector<ConnectedEdges> connected_border_edges;
    while (border_edges.size()) {
        ConnectedEdges new_set = get_connected_border(border_edges);
        if (new_set.size()) {
            connected_border_edges.push_back(new_set);
        }
    }
    printf("Found %lu borders\n", connected_border_edges.size());

    std::vector<Faces> groups_of_faces;
    groups_of_faces.push_back(faces);
    for (ConnectedEdges border_edges : connected_border_edges) {
        try {
            Faces added_faces = close_hole(
                border_edges, reader.GetShapes()[0], reader.GetAttrib(),
                normal_index_of, face_index_of);
            printf("Added %lu faces\n", added_faces.size() / 3);
            groups_of_faces.push_back(added_faces);
        } catch (std::logic_error const &e) {
            printf("%s\n", e.what());
        }
    }
    export_mesh("repaired.obj", vertices, normals, groups_of_faces);

    return 0;
}

// Author: Blurgy <gy@blurgy.xyz>
// Date:   May 21 2021, 10:49 [CST]
