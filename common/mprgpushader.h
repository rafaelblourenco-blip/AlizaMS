#ifndef ALIZA_MPR_GPU_SHADER_H
#define ALIZA_MPR_GPU_SHADER_H
// Prototype scalar projection; not connected to the clinical viewer until
// GPU/CPU equivalence and display generation synchronization are validated.
// first_index is the center of output pixel (0,0), in relative voxel indices.
namespace mpr_gpu {
static const char vertex_shader[] = R"GLSL(#version 150 core
void main() {
    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
)GLSL";
static const char projection_shader[] = R"GLSL(#version 150 core
uniform sampler3D volume;
uniform vec3 volume_size;
uniform vec3 first_index;
uniform vec3 index_du;
uniform vec3 index_dv;
uniform vec3 normal_per_mm;
uniform float sample_step_mm;
uniform int half_samples;
uniform int mode; // 1 maximum, 2 minimum, 3 mean
out vec2 projection; // scalar value, validity
void main() {
    vec2 pixel = floor(gl_FragCoord.xy);
    vec3 center = first_index + pixel.x * index_du + pixel.y * index_dv;
    float value = 0.0;
    int count = 0;
    for (int i = -half_samples; i <= half_samples; ++i) {
        vec3 p = center + float(i) * sample_step_mm * normal_per_mm;
        if (any(lessThan(p, vec3(-0.5))) || any(greaterThanEqual(p, volume_size - vec3(0.5)))) continue;
        float sample_value = texture(volume, (p + vec3(0.5)) / volume_size).r;
        if (isnan(sample_value) || isinf(sample_value)) continue;
        ++count;
        if (count == 1) value = sample_value;
        else if (mode == 1) value = max(value, sample_value);
        else if (mode == 2) value = min(value, sample_value);
        else value = value * (1.0 - 1.0 / float(count)) + sample_value / float(count);
    }
    projection = vec2(value, count == 0 ? 0.0 : 1.0);
}
)GLSL";
}
#endif
