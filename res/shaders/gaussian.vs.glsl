#version 430 core

#define POS_IDX 0
#define NOR_IDX 3
#define SHS_IDX 6
#define OPA_IDX 54
#define SCA_IDX 55
#define ROT_IDX 58
#define SH_DIM 3

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

out vec3 outColor;
out float opacity;
out vec3 conic;
out vec2 coordxy;

mat3 computeCov3D(vec4 rots, vec3 scales) {

  //float scaleMod = scaleMod;

  // vec3 firstRow = vec3(
  //   1.f - 2.f * (rots.z * rots.z + rots.w * rots.w),
  //   2.f * (rots.y * rots.z - rots.x * rots.w),      
  //   2.f * (rots.y * rots.w + rots.x * rots.z)       
  // );

  // vec3 secondRow = vec3(
  //   2.f * (rots.y * rots.z + rots.x * rots.w),       
  //   1.f - 2.f * (rots.y * rots.y + rots.w * rots.w), 
  //   2.f * (rots.z * rots.w - rots.x * rots.y)        
  // );

  // vec3 thirdRow = vec3(
  //   2.f * (rots.y * rots.w - rots.x * rots.z),       
  //   2.f * (rots.z * rots.w + rots.x * rots.y),     
  //   1.f - 2.f * (rots.y * rots.y + rots.z * rots.z) 
  // );

  // rots = (x, y, z, w)
  float x = rots.y, y = rots.z, z = rots.w, r = rots.x;
  float xx = x*x, yy = y*y, zz = z*z;
  float xy = x*y, xz = x*z, yz = y*z;
  float rx = r*x, ry = r*y, rz = r*z;

  mat3 rotMatrix = mat3(
    1.0 - 2.0*(yy + zz),  2.0*(xy - rz),       2.0*(xz + ry),
    2.0*(xy + rz),        1.0 - 2.0*(xx + zz), 2.0*(yz - rx),
    2.0*(xz - ry),        2.0*(yz + rx),       1.0 - 2.0*(xx + yy)
  );

  //rotMatrix = transpose(rotMatrix);

  mat3 scaleMatrix = mat3(
    scaleMod * scales.x, 0, 0, 
    0, scaleMod * scales.y, 0,
    0, 0, scaleMod * scales.z
  );

  // mat3 rotMatrix = mat3(
  //   firstRow,
  //   secondRow,
  //   thirdRow
  // );

  mat3 mMatrix = scaleMatrix * rotMatrix;

  mat3 sigma = transpose(mMatrix) * mMatrix;
  //mat3 sigma = mMatrix * transpose(mMatrix);
  return sigma;
};

out float quadId;
out float distance;
uniform int instanceID;
uniform int loop;
void main()
{
    quadId = float(sortedGaussianIdx[loop * 1000 + gl_InstanceID]);
    int total_dim = 62;//3 + 4 + 3 + 1 + SH_DIM;
    int start = int(quadId) * total_dim;
    quadId = float(loop * 1000 + gl_InstanceID);

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

    mat3 T = mat3(view) * J;
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

    float trace = cov2d[0][0] + cov2d[1][1];  
    float mid = trace * 0.5;
    float discriminant = max(0.1, mid * mid - det);
    float lambda1 = mid + sqrt(discriminant);
    float lambda2 = mid - sqrt(discriminant);
    // float maxAspectRatio = 3.0;
    // if(lambda1 > lambda2 * maxAspectRatio * maxAspectRatio) {
    //     lambda1 = lambda2 * maxAspectRatio * maxAspectRatio;  
    // }
    float radius = 3.0 * sqrt(max(lambda1, lambda2));
    
    // Project quad into screen space
    vec2 quadwh_scr = vec2(radius, radius); //vec2(3.f * sqrt(cov2d[0][0]), 3.f * sqrt(cov2d[1][1]));

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
    const float SH_C0 = 0.28209479177387814; // 0.5 * sqrt(1/PI)
    outColor = (0.5 + SH_C0 * colorVal);
    outColor = max(vec3(0.0), outColor);
    // outColor = colorVal;
    opacity = gData[start + OPA_IDX];
}