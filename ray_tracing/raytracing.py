"""
MIT License
Copyright (c) 2017 Cyrille Rossant
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
"""

from turtle import distance
import matplotlib.pyplot as plt
import numpy as np

w = 400
h = 300


def normalize(x):
    x /= np.linalg.norm(x)
    return x


def intersect_plane(O, D, P, N):
    # Return the distance from O to the intersection of the ray (O, D) with the
    # plane (P, N), or +inf if there is no intersection.
    # O and P are 3D points, D and N (normal) are normalized vectors.
    denom = np.dot(D, N)
    if np.abs(denom) < 1e-6:
        return np.inf
    d = np.dot(P - O, N) / denom
    if d < 0:
        return np.inf
    return d


def intersect_sphere(O, D, S, R):
    # Return the distance from O to the intersection of the ray (O, D) with the
    # sphere (S, R), or +inf if there is no intersection.
    # O and S are 3D points, D (direction) is a normalized vector, R is a scalar.
    a = np.dot(D, D)
    OS = O - S
    b = 2 * np.dot(D, OS)
    c = np.dot(OS, OS) - R * R
    disc = b * b - 4 * a * c
    if disc > 0:
        distSqrt = np.sqrt(disc)
        q = (-b - distSqrt) / 2.0 if b < 0 else (-b + distSqrt) / 2.0
        t0 = q / a
        t1 = c / q
        t0, t1 = min(t0, t1), max(t0, t1)
        if t1 >= 0:
            return t1 if t0 < 0 else t0
    return np.inf


def intersect_triangle(O, D, T, N):
    # Return the distance from O to the intersection of the ray (O, D) with the
    # traingle (T, N), or +inf if there is no intersection.
    # O and T are 3D points, D and N (normal) are normalized vectors.
    # implements the moller trumbore intersection algorithm slightly modified https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/barycentric-coordinates
    plane_intersection = intersect_plane(O[0], D, np.array(T[0]), N)
    if (plane_intersection == np.inf):
        return np.inf  # if it doesnt intersect plane, it doesnt intersect triangle in the plane

    # we calculate the intersection point with the plane
    plane_intersection_point = O + D * plane_intersection
    # triangle vertices and vectors
    vert0 = np.array(T[0])
    vert1 = np.array(T[1])
    vert2 = np.array(T[2])
    vector20 = vert2 - vert0
    vector10 = vert1 - vert0
    vectorplane2 = plane_intersection_point - vert0
    # calculates the scalar products for the triangle so we can compute the barycentric coordinates
    scalar1 = np.dot(vector20, vector20)
    scalar2 = np.dot(vector20, vector10)
    scalar3 = np.dot(vector20, vectorplane2)
    scalar4 = np.dot(vector10, vector10)
    scalar5 = np.dot(vector10, vectorplane2)
    #  we calculate the barycentric coordinates for the triangle
    d = scalar1 * scalar4 - scalar2 * scalar2
    u = (scalar4 * scalar3 - scalar2 * scalar5) / d
    v = (scalar1 * scalar5 - scalar2 * scalar3) / d
    # if the point is outside the triangle we return np.inf, otherwise the intersection with the previously calculated plane
    if (u >= 0) and (v >= 0) and (u + v < 1):
        return plane_intersection
    else:
        return np.inf

def intersect_triangles_strip(O, D, T):

    min_distance = np.inf
    distances_list = []

    for triangle in range(len(T)):
        distances_list.append(intersect_triangle(O, D, T[triangle]['position'], T[triangle]['normal']))
    
    min_distance = min(distances_list)

    return min_distance


def intersect(O, D, obj):
    if obj['type'] == 'plane':
        return intersect_plane(O, D, obj['position'], obj['normal'])
    elif obj['type'] == 'sphere':
        return intersect_sphere(O, D, obj['position'], obj['radius'])
    elif obj['type'] == 'triangle':
        return intersect_triangle(O, D, obj['position'], obj['normal'])
    elif obj['type'] == 'triangles_strip':
        return intersect_triangles_strip(O, D, obj['elements'])


def get_normal(obj, M):
    # Find normal.
    if obj['type'] == 'sphere':
        N = normalize(M - obj['position'])
    elif obj['type'] == 'plane':
        N = obj['normal']
    elif obj['type'] == 'triangle':
        N = obj['normal']
    elif obj['type'] == 'triangles_strip':
        N = obj['normal']
    return N


def get_color(obj, M):
    color = obj['color']
    if not hasattr(color, '__len__'):
        color = color(M)
    return color


def trace_ray(rayO, rayD):
    # Find first point of intersection with the scene.
    t = np.inf
    for i, obj in enumerate(scene):
        t_obj = intersect(rayO, rayD, obj)
        if t_obj < t:
            t, obj_idx = t_obj, i
    # Return None if the ray does not intersect any object.
    if t == np.inf:
        return
    # Find the object.
    obj = scene[obj_idx]
    
    if obj['type'] == 'triangles_strip':
        t_2 = np.inf
        # Si es un grupo de triangulos y iteramos sobre ellos
        # para obtener el traingulo que impacta con el rayo
        for j, obj_2 in enumerate(obj['elements']):
            t_obj_2 = intersect(rayO, rayD, obj_2)
            if t_obj_2 < t_2:
                t_2, obj_idx_2 = t_obj_2, j
        # Return None if the ray does not intersect any object.
        if t_2 == np.inf:
            return
        # Find the object.
        obj = obj['elements'][obj_idx_2]

    # Find the point of intersection on the object.
    M = rayO + rayD * t
    # Find properties of the object.
    N = get_normal(obj, M)
    color = get_color(obj, M)

    color_ray = ambient

    # Iteramos sobre cada una de las luces en en el array de posiciones de luces.
    for pos in range(len(light_position_array)):
        toL = normalize(light_colors_array[pos] - M)
        toO = normalize(O - M)
        # Shadow: find if the point is shadowed or not.
        l = [
            intersect(M + N * .0001, toL, obj_sh)
            for k, obj_sh in enumerate(scene) if k != obj_idx
        ]
        if l and min(l) < np.inf:
            pass
        else:
            # Lambert shading (diffuse).
            color_ray += obj.get('diffuse_c', diffuse_c) * max(
                np.dot(N, toL), 0) * color
            # Blinn-Phong shading (specular).
            color_ray += obj.get('specular_c', specular_c) * max(
                np.dot(N, normalize(toL + toO)),
                0)**specular_k * light_colors_array[pos]

    return obj, M, N, color_ray


def add_sphere(position, radius, color):
    return dict(type='sphere',
                position=np.array(position),
                radius=np.array(radius),
                color=np.array(color),
                reflection=.5)


def add_plane(position, normal):
    return dict(type='plane',
                position=np.array(position),
                normal=np.array(normal),
                color=lambda M: (color_plane0 if (int(M[0] * 2) % 2) ==
                                 (int(M[2] * 2) % 2) else color_plane1),
                diffuse_c=.75,
                specular_c=.5,
                reflection=.25)


def add_triangle(position, color):
    # Se calcula la normal utilizando el producto vectorial para triangulos utilizado en OpenGL https://www.khronos.org/opengl/wiki/Calculating_a_Surface_Normal
    return dict(type='triangle',
                color=np.array(color),
                position=np.array(position),
                normal=np.cross(np.subtract(position[1], position[0]),
                                np.subtract(position[2], position[0])),
                reflection=.3)

def add_triangles_strip(position, color):
    
    if len(position) >= 3:
        triangles = []
        normal = []
        
        triangles.append(add_triangle(
            np.array([position[0], position[1], position[2]]), color))

        normal.append(np.cross(np.subtract(position[1], position[0]),
                                np.subtract(position[2], position[0])))

        for i in range(3, len(position), 1):
            triangles.append(add_triangle(
                [position[i - 1], position[i - 2], position[i]], color))
            normal.append(np.cross(np.subtract(position[i - 1], position[i]),
                                np.subtract(position[i - 2], position[i])))

        return dict(type='triangles_strip',
                    color=np.array(color),
                    elements=np.array(triangles),
                    normal=normal,
                    reflection=.1)
    else:
        return

# List of objects.
color_plane0 = 1. * np.ones(3)
color_plane1 = 0. * np.ones(3)
scene = [
    add_sphere([.75, .1, 1.], .6, [0., 0., 1.]),
    add_sphere([-.75, .1, 2.25], .6, [.5, .223, .5]),
    add_sphere([-2.75, .1, 3.5], .6, [1., .572, .184]),
    add_plane([0., -.5, 0.], [0., 1., 0.]),
    add_triangle([[-0.4, 0.4, 3.], [0., -0.5, 3.], [0.5, 0.5, 3.]],
                 [1., 0.5, 0.]),
    add_triangles_strip([[-0.6, 1.8, 3.], [0., 1.5, 3.], [0.5, 1.9, 3.], [1.6,  1.3,  3. ]],
                 [1., 0., 0.]),
]

# Light position and color arrays.
light_position_array = np.array([[5., 5., -10.], [3., 3., 1.],
                                 [-10., 4., -20.]])
light_colors_array = np.array([[3., 1., 1.], [2., 2., 1.], [3., 3., 3.]])

# Default light and material parameters.
ambient = .05
diffuse_c = 1.
specular_c = 1.
specular_k = 50

depth_max = 5  # Maximum number of light reflections.
col = np.zeros(3)  # Current color.
O = np.array([0., 0.35, -1.])  # Camera.
Q = np.array([0., 0., 0.])  # Camera pointing to.
img = np.zeros((h, w, 3))

r = float(w) / h
# Screen coordinates: x0, y0, x1, y1.
S = (-1., -1. / r + .25, 1., 1. / r + .25)

# Loop through all pixels.
for i, x in enumerate(np.linspace(S[0], S[2], w)):
    if i % 10 == 0:
        print(i / float(w) * 100)
    for j, y in enumerate(np.linspace(S[1], S[3], h)):
        col[:] = 0
        Q[:2] = (x, y)
        D = normalize(Q - O)
        depth = 0
        rayO, rayD = O, D
        reflection = 1.
        # Loop through initial and secondary rays.
        while depth < depth_max:
            traced = trace_ray(rayO, rayD)
            if not traced:
                break
            obj, M, N, col_ray = traced
            # Reflection: create a new ray.
            rayO, rayD = M + N * .0001, normalize(rayD -
                                                  2 * np.dot(rayD, N) * N)
            depth += 1
            col += reflection * col_ray
            reflection *= obj.get('reflection', 1.)
        img[h - j - 1, i, :] = np.clip(col, 0, 1)

plt.imsave('fig.png', img)
