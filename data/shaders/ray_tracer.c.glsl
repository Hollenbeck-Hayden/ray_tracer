layout(local_size_x = 1, local_size_y = 1) in;
layout(rgba32f, binding=0) uniform image2D img_output;

/*	----- Shader Uniforms -----	*/
uniform vec3 eye;			// Camera position
uniform vec3 scene_center;		// Point camera is looking at
uniform vec3 raw_up;			// Up axis (before processing)
uniform vec2 screen_size;		// Window viewing size
uniform float near;			// Distance to window




/*	----- Global Constants -----	*/
// Intersection parameters
const float MIN_TRACE_DIST = 0.001;			// Initial offset factor
const float MAX_TRACE_DIST = 100.0;			// Maximum distance to render

// Scene parameters
const vec4 BACKGROUND_COLOR = vec4(0,0,0,1);		// Color if a ray intersects with nothing
const float AMBIENT = 0.3;				// Ambient light intensity

// Parameters to determine trace function
const int TRACE_DEPTH = 3;				// Number of subtraces to perform
const int TREE_SIZE = (1 << (TRACE_DEPTH+1))-1;
const int SUBTREE_SIZE = (1 << TRACE_DEPTH)-1;

// Subpixel sampling parameters
const int SUPERSAMPLE_X_COUNT = 1;
const int SUPERSAMPLE_Y_COUNT = 1;
const int SUPERSAMPLE_COUNT = SUPERSAMPLE_X_COUNT * SUPERSAMPLE_Y_COUNT;

// Index of refraction outside objects
const float VOID_INDEX_OF_REFRACTION = 1;

/*	----- Structs -----	*/

/*
* Describes how light interacts with an object.
*/
struct Material
{
	// Reflective parameters
	bool reflective;
	float k_reflect;

	// Refractive parameters
	bool refractive;
	float index_of_refraction;

	// Phong illumination parameters
	vec4 k_ambient;
	vec4 k_diffuse;
	vec4 k_specular;
	float shininess;
};

/*
* Contains all relevant information about an incident ray intersecting with the surface of an
* object. 
*/
struct Intersection
{
	float t;		// Length along ray intersection occurs
	vec3 point;		// Point on surface where intersection occurs
	vec3 normal;		// Surface normal at intersection
	vec3 incident;		// Incident light vector
	Material material;	// Surface material

	bool doesIntersect;	// Whether an intersection occurs or not
};

/*
* A monochromatic light source in the scene.
*/
struct Light
{
	vec3 position;
	vec4 color;
};

/*
* A sphere in the scene.
*/
struct Sphere
{
	vec3 center;
	float radius;
	Material material;
};

/*
* An infinite, flat plane in the scene.
*/
struct Plane
{
	vec3 point;		// Point in the plane
	vec3 n;			// Normal vector in the plane
	Material material;
};

/*	----- Global Arrays -----	*/

Light light_sources[2];
Sphere spheres[10];
Plane planes[1];


/*	----- Intersection Methods -----	*/

/*
* Gives the point of the ray parameterized by t.
*/
vec3 ray_at(vec3 ray, vec3 origin, float t)
{
	return origin + ray * t;
}

/*
* Determines if a sphere and a ray intersect.
*/
Intersection sphere_intersect(vec3 ray, vec3 origin, Sphere sphere)
{
	vec3 u = sphere.center - origin;
	float x = dot(ray, u);
	float det = x*x - dot(u, u) + sphere.radius * sphere.radius;

	Intersection intersection;
	if (det < 0)
	{
		// No intersection
		intersection.doesIntersect = false;
		return intersection;
	}
	else
	{
		// Intersection
		float s = sqrt(det);

		// Only give closest intersection
		if (x < s)
			intersection.t = x + s;
		else
			intersection.t = x - s;

		intersection.doesIntersect = true;
		intersection.material = sphere.material;
		intersection.incident = ray;

		// Be careful to calculate point before normal!
		intersection.point = ray_at(ray, origin, intersection.t);
		intersection.normal = normalize(intersection.point - sphere.center);

		return intersection;
	}
}

/*
* Determines if a ray and a plane intersect.
*/
Intersection plane_intersect(vec3 ray, vec3 origin, Plane plane)
{
	float cost = dot(ray, plane.n);

	Intersection intersection;
	if (abs(cost) < 0.005)
	{
		// Ray is parallel to the plane (assume zero thickness)
		intersection.doesIntersect = false;
		return intersection;
	}
	else
	{
		// Ray and plane intersect
		intersection.doesIntersect = true;
		intersection.t = dot(plane.point - origin, plane.n) / cost;

		intersection.point = ray_at(ray, origin, intersection.t);
		intersection.material = plane.material;
		intersection.normal = plane.n;
		intersection.incident = ray;

		return intersection;
	}
}

/*	----- Refraction Methods -----	*/

/*
* Determines the refraction ratio eta between a medium and the void for the given intersection.
*/
float refraction_ratio(Intersection intersection)
{
	float n1 = VOID_INDEX_OF_REFRACTION;
	float n2 = intersection.material.index_of_refraction;
	
	if (dot(intersection.incident, intersection.normal) < 0)
		return n1 / n2;
	else
		return n2 / n1;
}

/*
* Determines the reflection and transmission coefficients for a given intersection via
* Fresnel's equation.
* Code adapted from scratchapixel.com's tutorial:
* https://www.scratchapixel.com/code.php?id=8&origin=/lessons/3d-basic-rendering/ray-tracing-overview
*/
float fresnel(Intersection intersection)
{
	float cosi = clamp(-1, 1, dot(intersection.incident, intersection.normal));
	float etat = 1;
	float etai = refraction_ratio(intersection);

	float sint = etai / etat * sqrt(max(0, 1 - cosi * cosi));
	if (sint >= 1)
	{
		return 1;
	}
	else
	{
		float cost = sqrt(max(0, 1 - sint * sint));
		cosi = abs(cosi);
		float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
		float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
		return (Rs * Rs + Rp * Rp) / 2;
	}
}

/*	----- Tracing Methods -----	*/

/*
* Determines the intersection of the specified ray and the closest object. 
*/
Intersection intersect(vec3 ray, vec3 origin)
{
	// Offset origin to prevent hitting the same surface due to floating point errors
	origin += MIN_TRACE_DIST * ray;

	Intersection closest;
	closest.t = MAX_TRACE_DIST;
	closest.doesIntersect = false;

	// Iterate over spheres
	for (int i = 0; i < spheres.length(); i++)
	{
		Intersection intersection = sphere_intersect(ray, origin, spheres[i]);
		if (intersection.doesIntersect && intersection.t >= 0 && intersection.t < closest.t)
		{
			closest = intersection;
		}
	}

	// Iterate over planes
	for (int i = 0; i < planes.length(); i++)
	{
		Intersection intersection = plane_intersect(ray, origin, planes[i]);
		if (intersection.doesIntersect && intersection.t >= 0 && intersection.t < closest.t)
		{
			closest = intersection;
		}
	}

	return closest;
}

/*
* Calculates the contribution of light sources directly illuminating a point on the surface of an 
* object. Uses the Phong reflection model. 
*/
vec4 direct_illumination(Intersection intersection)
{
	// Ambient light contribution
	vec4 color = AMBIENT * intersection.material.k_ambient;

	for (int i = 0; i < light_sources.length(); i++)
	{
		vec3 L = light_sources[i].position - intersection.point;
		Intersection blocking = intersect(normalize(L), intersection.point);

		// Add light only if there's nothing blocking it
		if (length(L) < blocking.t)
		{
			// Phong reflection equation
			L = normalize(L);
			vec3 N = intersection.normal;
			vec3 R = reflect(-L, N);
			vec3 V = -intersection.incident;

			float cost = dot(L, N);
			if (cost > 0)
			{
				color += cost * intersection.material.k_diffuse * light_sources[i].color;
				color += intersection.material.k_specular * pow(max(0, dot(R, V)), intersection.material.shininess) * light_sources[i].color;
			}
		}
	}

	return color;
}

/*
* Gives the index of a reflected ray in the ray tree.
*/
int reflected(int i)
{
	return 2*i+1;
}

/*
* Gives the index of a refracted ray in the ray tree.
*/
int refracted(int i)
{
	return 2*i+2;
}

/*
* Checks whether a vector is 0 within a tolerance.
*/
bool is_zero(vec3 v)
{
	return length(v) < 0.00001;
}

/*
* Performs a trace of the specified ray direction starting at the origin.
*/
vec4 trace(vec3 ray, vec3 origin)
{
	Intersection intersection_tree[TREE_SIZE];
	vec4 color_tree[TREE_SIZE];

	/*	Build ray tree	*/

	intersection_tree[0] = intersect(ray, origin);

	for (int i = 0; i < SUBTREE_SIZE; i++)
	{
		if (intersection_tree[i].doesIntersect)
		{
			// Propagate refractive and reflective rays
			if (intersection_tree[i].material.refractive)
			{
				// Determine if the incident ray is inside or outside a material
				vec3 normal = intersection_tree[i].normal;

				if (dot(intersection_tree[i].incident, intersection_tree[i].normal) > 0)
				{
					normal *= -1;
				}

				vec3 ray = refract(intersection_tree[i].incident, normal, refraction_ratio(intersection_tree[i]));

				// Don't refract if there's total internal reflection
				if (!is_zero(ray))
				{
					intersection_tree[refracted(i)] = intersect(ray, intersection_tree[i].point);
				}
				else
				{
					intersection_tree[refracted(i)].doesIntersect = false;
				}

				intersection_tree[reflected(i)] = intersect(reflect(intersection_tree[i].incident, normal), intersection_tree[i].point);
			}
			else if (intersection_tree[i].material.reflective)
			{
				intersection_tree[reflected(i)] = intersect(reflect(intersection_tree[i].incident, intersection_tree[i].normal), intersection_tree[i].point);
				intersection_tree[refracted(i)].doesIntersect = false;
			}
			else
			{
				intersection_tree[reflected(i)].doesIntersect = false;
				intersection_tree[refracted(i)].doesIntersect = false;
			}
		}
		else
		{
			intersection_tree[reflected(i)].doesIntersect = false;
			intersection_tree[refracted(i)].doesIntersect = false;
		}
	}


	/*	Back propagate up the tree	*/

	for (int i = TREE_SIZE-1; i >= 0; i--)
	{
		if (intersection_tree[i].doesIntersect)
		{
			// Only illuminate non-refractive surfaces
			if (!intersection_tree[i].material.refractive)
			{
				color_tree[i] = direct_illumination(intersection_tree[i]);
			}

			// Add any refractive / reflective contributions
			if (i < SUBTREE_SIZE)
			{
				if (intersection_tree[i].material.refractive)
				{
					float k = fresnel(intersection_tree[i]);
					color_tree[i] = k * color_tree[reflected(i)] + (1-k) * color_tree[refracted(i)];
				}
				else if (intersection_tree[i].material.reflective)
				{
					color_tree[i] *= (1.0-intersection_tree[i].material.k_reflect);
					color_tree[i] += intersection_tree[i].material.k_reflect * color_tree[reflected(i)];
				}
			}
		}
		else
		{
			color_tree[i] = BACKGROUND_COLOR;
		}
	}

	return color_tree[0];
}






/*	----- Ray Sampling -----	*/

/*
* Uniformly samples the pixel for an array of rays to trace
*/
vec3[SUPERSAMPLE_COUNT] get_ray_subpixel_sampling(ivec2 pixel)
{
	// Determine screen coordinates
	vec2 tex_size = vec2(800, 800);
	vec2 pixel_pos = vec2(pixel);

	vec3 center_axis = normalize(scene_center - eye);
	vec3 right = normalize(cross(center_axis, raw_up));
	vec3 up = cross(right, center_axis);

	vec3 plane_center = center_axis * near;
	vec3 screen_x = mix(-screen_size.x / 2, screen_size.x / 2, pixel_pos.x / tex_size.x) * right;
	vec3 screen_y = mix(-screen_size.y / 2, screen_size.y / 2, pixel_pos.y / tex_size.y) * up;

	// Position of the pixel in world space
	vec3 plane_pos = plane_center + screen_x + screen_y;

	// Uniformly sample grid of the pixel
	vec2 pixel_size = vec2(screen_size.x / tex_size.x, screen_size.y / tex_size.y);
	vec3 rays[SUPERSAMPLE_COUNT];

	for (int i = 0; i < SUPERSAMPLE_X_COUNT; i++)
	{
		for (int j = 0; j < SUPERSAMPLE_Y_COUNT; j++)
		{
			vec2 subpixel_pos = vec2(pixel_size.x * i / SUPERSAMPLE_X_COUNT, pixel_size.y * j / SUPERSAMPLE_Y_COUNT);
			rays[3*i+j] = normalize(plane_pos + subpixel_pos.x * right + subpixel_pos.y * up);
		}
	}

	return rays;
}

/*	----- Factory -----	*/

Material generate_material(vec4 color, float k_ambient, float k_diffuse, float k_specular, float shininess)
{
	Material m;
	m.k_ambient = k_ambient * color;
	m.k_diffuse = k_diffuse * color;
	m.k_specular = k_specular * color;
	m.shininess = shininess;
	m.reflective = false;
	m.refractive = false;

	return m;
}

Material generate_std_material(vec4 color)
{
	return generate_material(color, 1, 0.4, 0.6, 4);
}


/*	----- Main -----	*/

void main()
{
	// Which pixel we're working on
	ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);

	/*	----- Build Scene -----	*/

	/* Build lights */
	light_sources[0].position = vec3(0, 3.5, 0);
	light_sources[0].color = vec4(1,1,1,1);

	light_sources[1].position = vec3(3, 1, 2);
	light_sources[1].color = vec4(1,1,1,1);

	/* Build spheres */

	float radius = 0.5;
	float spacing = 2.1 * radius;

	spheres[0].center = vec3(0,1.6*radius-1,0);
	spheres[0].radius = radius;
	spheres[0].material = generate_std_material(vec4(1,0,0,1));
	spheres[0].material.refractive = true;
	spheres[0].material.index_of_refraction = 1.53;

	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 2; j++)
		{
			spheres[2*i+j+1].center = vec3(spacing * (i-0.5), -1, spacing * (j-0.5));
			spheres[2*i+j+1].radius = radius;
			spheres[2*i+j+1].material = generate_std_material(vec4(1,0,0,1));
			spheres[2*i+j+1].material.refractive = true;
			spheres[2*i+j+1].material.index_of_refraction = 1.53;
		}
	}

	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 2; j++)
		{
			spheres[2*i+j+5].center = vec3(2*spacing * (i-0.5), -1-1.6*radius, 2*spacing * (j-0.5));
			spheres[2*i+j+5].radius = radius;
			spheres[2*i+j+5].material = generate_std_material(vec4(1,0,0,1));
			spheres[2*i+j+5].material.refractive = true;
			spheres[2*i+j+5].material.index_of_refraction = 1.53;
		}
	}

	/* Build planes as a stage */
	planes[0].point = vec3(0, -3, 0);
	planes[0].n = vec3(0, 1, 0);
	planes[0].material = generate_std_material(vec4(0.25,1,0,1));

	/*	----- Sampling -----	*/
	vec3 rays[SUPERSAMPLE_COUNT] = get_ray_subpixel_sampling(pixel);
	vec4 color = vec4(0,0,0,1);
	for (int i = 0; i < SUPERSAMPLE_COUNT; i++)
	{
		color += trace(rays[i], eye);
	}
	color /= SUPERSAMPLE_COUNT;

	/*	----- Output -----	*/
	imageStore(img_output, pixel, color);
}
