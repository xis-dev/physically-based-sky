#pragma once

#include "ext/matrix_clip_space.hpp"
#include "ext/matrix_float4x4.hpp"
#include "ext/matrix_transform.hpp"
#include "glm/vec3.hpp"


struct Camera
{
public:
	glm::vec3 position{};
	glm::vec3 direction{};
	glm::vec3 up{};
	
	float fov{ 45.0f };
	float near{ 0.1f };
	float far{ 100.0f };
	float aspect{};

	float yaw{};
	float pitch{};

	glm::vec3 setCamDir(float degYaw, float degPitch)
	{
		float yaw = glm::radians(degYaw);
		float pitch = glm::radians(degPitch);


		direction.x = cos(pitch) * cos(yaw);
		direction.y = sin(pitch);
		direction.z = cos(pitch) * sin(yaw);

		if (direction != glm::vec3(0.0f))
		{
			direction = glm::normalize(direction);
		}
		return direction;
	}

	glm::mat4 getViewMat() const
	{

		return glm::lookAt(position, position + glm::normalize(direction), glm::normalize(up));
	}

	glm::mat4 getProjectionMat() const
	{
		return glm::perspective(glm::radians(fov), aspect, near, far);
	}

};
