#version 430 core

#define POS_IDX 0
#define NOR_IDX 3
#define SHS_IDX 6
#define OPA_IDX 54
#define SCA_IDX 55
#define ROT_IDX 58
#define SH_DIM 3

#define TOTAL_SIZE 62

// #define POS_IDX 0
// #define NOR_IDX 3
// #define SHS_IDX 6
// #define OPA_IDX 9
// #define SCA_IDX 10
// #define ROT_IDX 13
// #define SH_DIM 3

// #define TOTAL_SIZE 17

const float SH_C0 = 0.28209479177387814f;
const float SH_C1 = 0.4886025119029199f;
const float SH_C2[5] = {
	1.0925484305920792f,
	-1.0925484305920792f,
	0.31539156525252005f,
	-1.0925484305920792f,
	0.5462742152960396f
};
const float SH_C3[7] = {
	-0.5900435899266435f,
	2.890611442640554f,
	-0.4570457994644658f,
	0.3731763325901154f,
	-0.4570457994644658f,
	1.445305721320277f,
	-0.5900435899266435f
};

layout(location = 0) in vec2 quadPosition;

layout(std430, binding=1) buffer gaussians_data{
    float gData[];
};

layout(std430, binding=2) buffer gaussians_order{
    int sortedGaussianIdx[];
};

vec3 get_vec3(int offset) {
	return vec3(gData[offset], gData[offset + 1], gData[offset + 2]);
};

vec4 get_vec4(int offset) {
	return vec4(gData[offset], gData[offset + 1], gData[offset + 2], gData[offset+3]);
};

uniform mat4 view;
uniform mat4 projection;
uniform vec4 hfov_focal;
uniform float scaleMod;
uniform vec3 campos;

out vec3 outColor;
out float opacity;
out vec3 conic;
out vec2 coordxy;
out vec3 worldPos;

mat3 computeCov3D(vec4 rots, vec3 scales) 
{
  float x = rots.y, y = rots.z, z = rots.w, r = rots.x;
  float xx = x*x, yy = y*y, zz = z*z;
  float xy = x*y, xz = x*z, yz = y*z;
  float rx = r*x, ry = r*y, rz = r*z;

  mat3 rotMatrix = mat3(
    1.0 - 2.0*(yy + zz),  2.0*(xy - rz),       2.0*(xz + ry),
    2.0*(xy + rz),        1.0 - 2.0*(xx + zz), 2.0*(yz - rx),
    2.0*(xz - ry),        2.0*(yz + rx),       1.0 - 2.0*(xx + yy)
  );

  mat3 scaleMatrix = mat3(
    scaleMod * scales.x, 0, 0, 
    0, scaleMod * scales.y, 0,
    0, 0, scaleMod * scales.z
  );

  mat3 mMatrix = scaleMatrix * rotMatrix;

  mat3 sigma = transpose(mMatrix) * mMatrix;
  return sigma;
};

vec3 computeColorFromSH(int idx, int deg, vec3 pos)
{
	// The implementation is loosely based on code for 
	// "Differentiable Point-Based Radiance Fields for 
	// Efficient View Synthesis" by Zhang et al. (2022)
	vec3 dir = pos - campos;
	dir = dir / length(dir);

    int start = idx * TOTAL_SIZE;
    vec3 sh[16];
    sh[0] = get_vec3(start + SHS_IDX);
    sh[1] = get_vec3(start + SHS_IDX + 3);
    sh[2] = get_vec3(start + SHS_IDX + 6);
    sh[3] = get_vec3(start + SHS_IDX + 9);
    sh[4] = get_vec3(start + SHS_IDX + 12);
    sh[5] = get_vec3(start + SHS_IDX + 15);
    sh[6] = get_vec3(start + SHS_IDX + 18);
    sh[7] = get_vec3(start + SHS_IDX + 21);
    sh[8] = get_vec3(start + SHS_IDX + 24);
    sh[9] = get_vec3(start + SHS_IDX + 27);
    sh[10] = get_vec3(start + SHS_IDX + 30);
    sh[11] = get_vec3(start + SHS_IDX + 33);
    sh[12] = get_vec3(start + SHS_IDX + 36);
    sh[13] = get_vec3(start + SHS_IDX + 39);
    sh[14] = get_vec3(start + SHS_IDX + 42);
    sh[15] = get_vec3(start + SHS_IDX + 45);
    vec3 result = SH_C0 * sh[0];

    if (deg > 0)
    {
        float x = dir.x;
        float y = dir.y;
        float z = dir.z;
        result = result - SH_C1 * y * sh[1] + SH_C1 * z * sh[2] - SH_C1 * x * sh[3];

		if (deg > 1)
		{
			float xx = x * x, yy = y * y, zz = z * z;
			float xy = x * y, yz = y * z, xz = x * z;
			result = result +
				SH_C2[0] * xy * sh[4] +
				SH_C2[1] * yz * sh[5] +
				SH_C2[2] * (2.0f * zz - xx - yy) * sh[6] +
				SH_C2[3] * xz * sh[7] +
				SH_C2[4] * (xx - yy) * sh[8];

			if (deg > 2)
			{
				result = result +
					SH_C3[0] * y * (3.0f * xx - yy) * sh[9] +
					SH_C3[1] * xy * z * sh[10] +
					SH_C3[2] * y * (4.0f * zz - xx - yy) * sh[11] +
					SH_C3[3] * z * (2.0f * zz - 3.0f * xx - 3.0f * yy) * sh[12] +
					SH_C3[4] * x * (4.0f * zz - xx - yy) * sh[13] +
					SH_C3[5] * z * (xx - yy) * sh[14] +
					SH_C3[6] * x * (xx - 3.0f * yy) * sh[15];
			}
		}
	}
	result += 0.5f;

	// // RGB colors are clamped to positive values. If values are
	// // clamped, we need to keep track of this for the backward pass.
	// clamped[3 * idx + 0] = (result.x < 0);
	// clamped[3 * idx + 1] = (result.y < 0);
	// clamped[3 * idx + 2] = (result.z < 0);
	return max(result, 0.0f);
}

out float quadId;
out float distance;
uniform int instanceID;
uniform int loop;

out float viewDepth;
void main()
{
    quadId = float(sortedGaussianIdx[gl_InstanceID]);
    int total_dim = TOTAL_SIZE;//3 + 4 + 3 + 1 + SH_DIM;
    int start = int(quadId) * total_dim;
    //quadId = float(gl_InstanceID);

    vec3 center = get_vec3(start + POS_IDX);
    vec3 colorVal = get_vec3(start + SHS_IDX);
    vec4 rotations = get_vec4(start + ROT_IDX);
    vec3 scale = get_vec3(start + SCA_IDX);

    mat3 cov3d = computeCov3D(rotations, scale);

    vec4 viewPos = view * vec4(center, 1.0);
    vec4 homPos =  projection * viewPos;
    distance = -viewPos.z;
    distance = distance / 10.0;
    distance = clamp(distance, 0.0, 1.0);
    viewDepth = -viewPos.z;
    worldPos = center;
    vec3 projPos = homPos.xyz / (homPos.w + 0.0000001);

    vec2 wh = 2 * hfov_focal.xy * hfov_focal.zw;

    float limx = 1.3 * hfov_focal.x;
    float limy = 1.3 * hfov_focal.y;

    float txtz = viewPos.x / viewPos.z;
    float tytz = viewPos.y / viewPos.z;

    float tx = min(max(txtz, -limx), limx) * viewPos.z;
    float ty = min(max(tytz, -limy), limy) * viewPos.z;

    if(any(greaterThan(abs(projPos.xyz), vec3(1.3)))) {
        gl_Position = vec4(-100.0, -100.0, -100.0, 1.0);
        return;
    }

    float focal_x = hfov_focal.z;
    float focal_y = hfov_focal.w;

    mat3 J = mat3(
        focal_x / viewPos.z, 0, -(focal_x * tx) / (viewPos.z * viewPos.z),
        0, focal_y / viewPos.z, -(focal_y * ty) / (viewPos.z * viewPos.z),
        0, 0, 0);

    mat3 T = transpose(mat3(view)) * J;
    mat3 cov2d = transpose(T) * cov3d * T;

    cov2d[0][0] += 0.3;
    cov2d[1][1] += 0.3;

    float det = cov2d[0][0] * cov2d[1][1] - cov2d[0][1] * cov2d[1][0];
    if(det < 1e-6 || isnan(det) || isinf(det)) {
        gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    float det_inv = 1.0 / det;
    conic = vec3(
        cov2d[1][1] * det_inv,
        -cov2d[0][1] * det_inv,
        cov2d[0][0] * det_inv
    );

    // float trace = cov2d[0][0] + cov2d[1][1];  
    // float mid = trace * 0.5;
    // float discriminant = max(0.1, mid * mid - det);
    // float lambda1 = mid + sqrt(discriminant);
    // float lambda2 = mid - sqrt(discriminant);

    // float radius = 3.0 * sqrt(max(lambda1, lambda2));
    
    // Project quad into screen space
    vec2 quadwh_scr = vec2(3.f * sqrt(cov2d[0][0]), 3.f * sqrt(cov2d[1][1]));

    // Convert screenspace quad to NDC
    vec2 quadwh_ndc = quadwh_scr / wh * 2.0;

    // Update gaussian's position w.r.t the quad in NDC
    projPos.xy = projPos.xy + quadPosition * quadwh_ndc;

    // // Calculate where this quad lies in pixel coordinates 
    coordxy = quadPosition * quadwh_scr;

    // Set position
    gl_Position = vec4(projPos.xyz, 1.0);
    //gl_Position = homPos;

    // Send values to fragment shader 
    //const float SH_C0 = 0.28209479177387814; // 0.5 * sqrt(1/PI)
    outColor = (0.5 + SH_C0 * colorVal);
    outColor = max(vec3(0.0), outColor);
    //outColor = computeColorFromSH(int(quadId), 2, center);
    // outColor = colorVal;
    opacity = gData[start + OPA_IDX];
}