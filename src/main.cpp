#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "shader.h"
#include "camera.h"
#include "PathResolver.h"

#include <iostream>
#include <numbers>
#include <format>

#include "gtc/type_ptr.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "stb_image.h"
#include "stb_image_resize2.h"

#define SCR_WIDTH 1600
#define SCR_HEIGHT 900

constexpr float PI = 3.14159265359f;
constexpr float TWO_PI = 2.0f * PI;


void imguiInit(GLFWwindow* window);
void imguiUse();
void imguiRender();
void frameBufferSizeCallback(GLFWwindow* window, int w, int h);
void mouseCallback(GLFWwindow* win, double xPos, double yPos);
void mouseInput(Camera& cam, float sensitivity, double xOffset, double yOffset);


constexpr float vertices[]
{
	 1.0f,  1.0f, 0.0f,
	 1.0f, -1.0f, 0.0f,
	-1.0f, -1.0f, 0.0f,
	-1.0f,  1.0f, 0.0f
};

constexpr unsigned indices[]
{
	0, 1, 2,
	2, 3, 0
};


double lastXPos{};
double lastYPos{};

double xOffset{};
double yOffset{};

bool firstMouseInput{true};
bool shiftLock{};

unsigned vao, vbo, ebo;

int frameCount{};


float animSpeed{ 2.0f };

float totalSimHours{};

float phaseBlend{.6f};

float solarTime{ 0.0f };

const float earthRad = (6360e3 + 1.0f);
glm::vec2 latLong{ 39.7f, 8.82f };
glm::vec2 sunAltAzi{20.0f, 270.0f};
glm::vec2 moonAltAzi{};

bool simulate{true};
bool useEphemerisPos{};


double jdStart{};

bool clouds{true};
glm::vec<3, int> startDate{28, 4, 2026};
glm::vec<3, int> currentDate{};
int dayOfTheYear{179};


glm::vec3 sunDir{};
glm::vec3 moonDir{};
Camera cam;

double julianDay(double d, int m, int y)
{
	if (m <= 2)
	{
		m = m + 12;
		y = y - 1;
	}

	// Using gregorian calender, B = 0 if julian
	const int A = floor(static_cast<float>(y) / 100.0f);
	const int B = 2 - A + (int)floor(static_cast<float>(A) / 4.0f);
	//return (int)(365.25 * y) + (int)(30.6001 * (m + 1)) - 15 + 1720996.5f + d + time / 24.0f;

	return floor((365.25 * (y + 4716))) + floor((30.6001 * (m + 1))) + d + B - 1524.5;

	// DT omitted for the time being so jd not jde
}

//double normalizeAngle(double angle, double max, double min)
//{
//	return min + fmod((fmod((angle - min), max) + max), max);
//}


bool isLeap(int y)
{
	return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}


void doyToDate(int& doy, int year, int& outDay, int& outMonth, int& outYear)
{
	outYear = year;

	while (true)
	{
		int daysInYear = isLeap(year) ? 366 : 365;
		if (doy <= daysInYear) break;
		doy -= daysInYear;
		outYear++;
	}

	int daysPerMonth[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
	if (isLeap(year)) daysPerMonth[1] = 29;

	outMonth = 0;
	int totalDays = doy;
	while (totalDays > daysPerMonth[outMonth])
	{
		totalDays -= daysPerMonth[outMonth];
		outMonth++;
	}

	outDay = totalDays;
	outMonth += 1;
}

void wrapDay(int& day, int& month, int& year)
{
	while (month > 12)
	{
		month -= 12;
		++year;
	}

	int daysPerMonth[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
	if (isLeap(year)) daysPerMonth[1] = 29;

	while (day > daysPerMonth[month - 1])
	{
		day -= daysPerMonth[month - 1];
		++month;
		if (month > 12)
		{
			month -= 12;
			++year;
		}
	}

}

void dateToDoy(int day, int month, int year, int& outDoy)
{
	wrapDay(day, month, year);

	outDoy = day;
	int daysPerMonth[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
	for (int i = 0; i < (month - 1); ++i)
	{
		outDoy += daysPerMonth[i];
	}
}
double wrap(double a, double min, double max)
{
	const double range = max - min;
	a = fmod(a - min, range);
	if (a < 0.0) a += range;
	return a + min;
}



glm::vec3 moonDirectionFromTime(double jd, float degLatitude, float degLongitude)
{

	float lat = glm::radians(degLatitude);
	float longitude = glm::radians(degLongitude);
	double t = (jd - 2451545.0) / 36525.0;

	// Moon mean longitude
	double Lp = 218.3164477 + 481267.88123421 * t - 0.0015786 * (t * t) + (t * t * t) / 538841.0 - (t * t * t * t) / 65194000.0;

	Lp = wrap(Lp, 0.0, 360.0);

	// Moon mean elongation
	double D = 297.8501921 + 445267.1114034 * t - 0.0018819 * (t * t) + (t * t * t) / 545868.0 - (t * t * t * t) / 113065000.0;

	D = wrap(D, 0.0, 360.0);


	// Sun mean anomaly
	double M = 357.5291092 + 35999.0502909 * t - 0.0001536 * (t * t) + (t * t * t) / 24490000.0;

	M = wrap(M, 0.0, 360.0);


	// Moon mean anomaly
	double Mp = 134.9633964 + 477198.8675055 * t + 0.0087414 * (t * t) + (t * t * t) / 69699.0 - (t * t * t * t) / 14712000.0;

	Mp = wrap(Mp, 0.0, 360.0);


	// Mean distance of moon from ascending node
	double F = 93.2720950 + 483202.0175233 * t - 0.0036539 * (t * t) - (t * t * t) / 3526000.0 + (t * t * t * t) / 863310000.0;

	F = wrap(F, 0.0, 360.0);


	// Eccentricity
	double E = 1 - 0.002516 * t - 0.0000074 * (t * t);
	//double A_1 = 119.75 + 131.849 * t;
	//double A_2 = 53.09 + 479264.290 * t;
	//double A_3 = 313.45 + 481266.484 * t;

	// Periodic terms of longitude
	double sumL = 6.289 * sin(glm::radians(Mp)) * E
				+ 1.274 * sin(glm::radians(2 * D + (-Mp))) * E
				+ 0.658 * sin(glm::radians(2 * D))
				+ 0.214 * sin(glm::radians(2 * Mp)) * (E * E)
				- 0.185 * sin(glm::radians(M)) * E
				- 0.114 * sin(glm::radians(2 * F));

	// Periodic terms of latitude
	double sumB = 5.128 * sin(glm::radians(F))
				+ 0.280 * sin(glm::radians(Mp + F)) * E
				+ 0.277 * sin(glm::radians(Mp - F)) * E
				+ 0.173 * sin(glm::radians(2 * D - F));

	// Ecliptic longitude
	double lambda = ((wrap(glm::radians(Lp + sumL), 0.0f, TWO_PI)));

	// Ecliptic latitude
	double beta = glm::radians(sumB);

	// Obliquity of eliptic
	double eps = glm::radians(23.0 + 26.0 / 60.0 + 21.448 / 3600.0 - (46.8150 * t + 0.00059 * t * t - 0.001813 * t * t * t) / 3600.0f);

	double x = cos(beta) * cos(lambda);
	double y = cos(eps) * cos(beta) * sin(lambda) - sin(eps) * sin(beta);
	double z = sin(eps) * cos(beta) * sin(lambda) + cos(eps) * sin(beta);

	double ra = atan2(y, x);
	double dec = atan2(z, sqrt(x * x + y * y));

	ra = wrap(ra, 0.0, TWO_PI);

	double theta0_deg = 280.46061837
		+ 360.98564736629 * (jd - 2451545.0)
		+ 0.000387933 * t * t
		- (t * t * t) / 38710000.0;

	theta0_deg = wrap(theta0_deg, 0.0, 360.0);
	double theta0 = glm::radians(theta0_deg);
	double theta = theta0 + longitude;
	double hourAngle = wrap(theta - ra, -PI, PI);



	double azimuth = (atan2((sin(hourAngle)), (sin(lat) * cos(hourAngle) - tan(dec) * cos(lat))));

	double sinAlt = sin(lat) * sin(dec) + cos(lat) * cos(dec) * cos(hourAngle);

	sinAlt = glm::clamp(sinAlt, -1.0, 1.0);
	double altitude = (asin(sinAlt));
	altitude = wrap(altitude, -PI + 0.1, PI - 0.1);

	azimuth = wrap(azimuth, 0.0, TWO_PI);
	moonAltAzi.x = static_cast<float>(glm::degrees(altitude));
	moonAltAzi.y = static_cast<float>(glm::degrees(azimuth));

	auto dir = glm::vec3(glm::cos(altitude) * glm::sin(azimuth),
		glm::sin(altitude),
		glm::cos(altitude) * glm::cos(azimuth));



	if (fabs(dir.x) < 1e-12f) dir.x = 0.0f;
	if (fabs(dir.y) < 1e-12f) dir.y = 0.0f;
	if (fabs(dir.z) < 1e-12f) dir.z = 0.0f;


	return glm::normalize(dir);
}


glm::vec3 sunDirectionFromTime(float time, int day, float degLatitude)
{

	const float lat = glm::radians(degLatitude);
	const float hourAngle = glm::radians(15.0f * (time - 12.0f));

	// Day angle
	const float g = 2 * PI * (float(day - 1) / 365.0f);

	const double dec = 0.006918 - 0.399912 * cos(g) + 0.070257 * sin(g) - 0.006758 * cos(2 * g) + 0.000907 * sin(2 * g) - 0.002697 * cos(3 * g) + 0.001480 * sin(3 * g);


	double sinAlt = sin(lat) * sin(dec) + cos(lat) * cos(dec) * cos(hourAngle);
	sinAlt = glm::clamp(sinAlt, -1.0, 1.0);
	double altitude = asin(sinAlt);
	altitude = wrap(altitude, -PI + 0.1, PI - 0.1);

	double azimuth = atan2((sin(hourAngle)), (sin(lat) * cos(hourAngle) - tan(dec) * cos(lat)));

	azimuth = wrap(azimuth, 0.0f, TWO_PI);
	sunAltAzi.x = static_cast<float>(glm::degrees(altitude));
	sunAltAzi.y = static_cast<float>(glm::degrees(azimuth));



	auto dir = glm::vec3(glm::cos(altitude) * glm::sin(azimuth),
		glm::sin(altitude),
		glm::cos(altitude) * glm::cos(azimuth));

	if (fabs(dir.x) < 1e-12f) dir.x = 0.0f;
	if (fabs(dir.y) < 1e-12f) dir.y = 0.0f;
	if (fabs(dir.z) < 1e-12f) dir.z = 0.0f;


	return glm::normalize(dir);

}

unsigned initTexture(const std::string& loc)
{
	unsigned id{};
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	int width, height, nrColCh;

	unsigned char* data = stbi_load(loc.c_str(), &width, &height, &nrColCh, 0);

	if (!data)
	{
		std::cout << "Stb failed to load img at: " << loc << "\n";
	}
	GLenum imageFormat{};

	switch (nrColCh)
	{
	case 1:
		imageFormat = GL_R;
		break;

	case 2:
		imageFormat = GL_RG;
		break;

	case 4:
		imageFormat = GL_RGBA;
		break;

		default:
			imageFormat = GL_RGB;
			break;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, imageFormat, GL_UNSIGNED_BYTE, data);

	stbi_image_free(data);

	glBindTexture(GL_TEXTURE_2D, 0);

	return id;
}

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Sky", nullptr, nullptr);

	if (!window)
	{
		std::cout << "Failed to init GLFW window, terminating. \n";
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, frameBufferSizeCallback);
	glfwSetCursorPosCallback(window, mouseCallback);
	///glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


	if (!gladLoadGLLoader((GLADloadproc)(glfwGetProcAddress)))
	{
		std::cout << "Failed to initialize glad.\n";
		glfwTerminate();
		return -1;
	}
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);
	glGenVertexArrays(1, &vao);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(0));
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);





	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "Incomplete framebuffer. \n";
		glfwTerminate();
		glDeleteBuffers(1, &vao);
		glDeleteBuffers(1, &vbo);
		glDeleteBuffers(1, &ebo);
		return -1;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);





	cam.position = glm::vec3(0.0f, 6360e3 + 1, 0.0f);
	// TODO: Camera direction with coord space
	cam.direction = glm::vec3(1.0f, 0.0f, 0.0f);
	cam.aspect = (float)SCR_WIDTH / SCR_HEIGHT;

	Shader baseShader{ "shaders/base.vert", "shaders/sky.frag" };
	baseShader.use();
	baseShader.setUniformi("u_ScrWidth", SCR_WIDTH);
	baseShader.setUniformi("u_ScrHeight", SCR_HEIGHT);
	baseShader.setUniformBlock("SharedInfo", 0);


	imguiInit(window);
	float lastFrameTime{};



	jdStart = julianDay(startDate.x, startDate.y, startDate.z);
	dateToDoy(startDate.x, startDate.y, startDate.z, dayOfTheYear);
	currentDate = startDate;
	while (!glfwWindowShouldClose(window))
	{

		float currentTime = glfwGetTime();
		float deltaTime = currentTime - lastFrameTime;
		lastFrameTime = currentTime;
		glfwPollEvents();
		imguiUse();

	//mouseInput(cam, 0.05f, xOffset, yOffset);


		totalSimHours += animSpeed * deltaTime;
		double jd{};
		if (simulate)
		{
			startDate = currentDate;
			doyToDate(dayOfTheYear, currentDate.z, currentDate.x, currentDate.y, currentDate.z);
			solarTime += animSpeed * deltaTime;
			if (solarTime > 24.0f)
			{
				solarTime = fmod(totalSimHours, 24.0f);
				++dayOfTheYear;
			}
			 jd = jdStart + totalSimHours / 24.0;

			sunDir = sunDirectionFromTime(solarTime, dayOfTheYear, latLong.x);
			moonDir = moonDirectionFromTime(jd, latLong.x, latLong.y);

		}
		else if (!simulate && useEphemerisPos)
		{
			jd = julianDay(currentDate.x, currentDate.y, currentDate.z) + solarTime/24.0f;

			sunDir = sunDirectionFromTime(solarTime, dayOfTheYear, latLong.x);
			moonDir = moonDirectionFromTime(jd, latLong.x, latLong.y);

		}
		else
		{
			sunDir = glm::vec3(glm::cos(glm::radians(sunAltAzi.x)) * glm::sin(glm::radians(sunAltAzi.y)),
							 glm::sin(glm::radians(sunAltAzi.x)),
							 glm::cos(glm::radians(sunAltAzi.x)) * glm::cos(glm::radians(sunAltAzi.y)));

			if (fabs(sunDir.x) < 1e-12f) sunDir.x = 0.0f;
			if (fabs(sunDir.y) < 1e-12f) sunDir.y = 0.0f;
			if (fabs(sunDir.z) < 1e-12f) sunDir.z = 0.0f;

			sunDir = glm::normalize(sunDir);

			moonDir = glm::vec3(glm::cos(glm::radians(moonAltAzi.x)) * glm::sin(glm::radians(moonAltAzi.y)),
			glm::sin(glm::radians(moonAltAzi.x)),
			glm::cos(glm::radians(moonAltAzi.x)) * glm::cos(glm::radians(moonAltAzi.y)));

			if (fabs(moonDir.x) < 1e-12f) moonDir.x = 0.0f;
			if (fabs(moonDir.y) < 1e-12f) moonDir.y = 0.0f;
			if (fabs(moonDir.z) < 1e-12f) moonDir.z = 0.0f;

			moonDir = glm::normalize(moonDir);
		}


		float ut = fmod(totalSimHours - latLong.y / 15.0f, 24.0f);
		glClearColor(0.53f, 0.0f, 0.42f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);


		cam.position = glm::vec3(earthRad * glm::cos(glm::radians(latLong.x)) * glm::sin(glm::radians(latLong.y)), earthRad * glm::sin(glm::radians(latLong.x)), earthRad * glm::cos(glm::radians(latLong.x)) * glm::cos(glm::radians(latLong.y)));

		const float eps = earthRad * 5e-8f;
		if (fabs(cam.position.x) < eps) cam.position.x = 0.0f;
		if (fabs(cam.position.y) < eps) cam.position.y = 0.0f;
		if (fabs(cam.position.z) < eps) cam.position.z = 0.0f;

		glm::vec3 worldUp = glm::normalize(cam.position);

		if (abs(glm::dot(cam.direction, worldUp)) > 0.999f)
		{
			worldUp = glm::vec3(0.0f, 0.0f, 1.0f);
			// if (abs(glm::dot(cam.direction, worldUp)) > 0.999f)
			// 	worldUp = glm::vec3(0.0f, 0.0f, 1.0f);
		}

		cam.up = worldUp;

		baseShader.use();
		baseShader.setUniformMat4("u_InvProjection", glm::inverse(cam.getProjectionMat()));
		baseShader.setUniformMat4("u_InvView", glm::inverse(cam.getViewMat()));

		baseShader.setUniformVec3("u_Observer", cam.position);
		baseShader.setUniformVec3("u_SunDir", sunDir);
		baseShader.setUniformVec3("u_MoonDir", moonDir);
		baseShader.setUniformf("u_Time", glfwGetTime());
		baseShader.setUniformi("u_Clouds", clouds);

		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(unsigned), GL_UNSIGNED_INT, nullptr);

		frameCount++;

		imguiRender();
		glfwSwapBuffers(window);
	}

	glfwTerminate();
	glDeleteBuffers(1, &vao);
	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &ebo);
	return 0;
}

void mouseInput(Camera& camera, float sensitivity, double offsetX, double offsetY)
{
	camera.yaw += (float)offsetX * sensitivity;
	camera.pitch += (float)offsetY * sensitivity;

	camera.pitch = glm::clamp(camera.pitch, -89.0f, 89.0f);

	camera.setCamDir(camera.yaw, camera.pitch);

	xOffset = 0.0f;
	yOffset = 0.0f;
}

void frameBufferSizeCallback(GLFWwindow* window, int w, int h)
{
	glViewport(0, 0, w, h);
	cam.aspect = (float)w / h;
}

void mouseCallback(GLFWwindow* win, double xPos, double yPos)
{
	if (firstMouseInput)
	{
		lastXPos = xPos;
		lastYPos = yPos;

		firstMouseInput = false;

		return;
	}

	xOffset = xPos - lastXPos;
	yOffset = lastYPos - yPos;

	lastXPos = xPos;
	lastYPos = yPos;
}

void imguiInit(GLFWwindow* window)
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);          // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
	ImGui_ImplOpenGL3_Init();
}

void imguiUse()
{
	// setup imgui for a new frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Sky");

	ImGui::Text("Clouds: ");
	ImGui::Checkbox("Visible", &clouds);

	if (!simulate)
	{
		if (ImGui::Button("Start Simulation", ImVec2(250.0, 30.0)))
		{
			simulate = true;
			totalSimHours = 0.0;
			solarTime = 0.0;
			startDate = currentDate;
			jdStart = julianDay(startDate.x, startDate.y, startDate.z);
		}
	}
	else
	{
		if (ImGui::Button("Stop Simulation", ImVec2(250.0, 30.0)))
		{
			simulate = false;
		}
	}
	ImGui::DragFloat("Simulation Speed", &animSpeed, 0.05f, 0.0f);

	ImGui::Text("Observer Position: ");
	ImGui::DragFloat("Latitude", &latLong.x, 0.1f, -90.0f, 90.0f);
	ImGui::DragFloat("Longitude", &latLong.y, 0.1f, -180.0f, 180.0f);

	ImGui::DragFloat("Solar Time", &solarTime, 0.1f, 0.0f, 24.0f);
	ImGui::DragInt("Day", &dayOfTheYear);

	if (ImGui::InputInt3("Start Date", glm::value_ptr(startDate), ImGuiInputTextFlags_EnterReturnsTrue))
	{
		wrapDay(startDate.x, startDate.y, startDate.z);
	}

	ImGui::Text((std::format("Current Date: {}, {}, {}",  std::to_string(currentDate.x), std::to_string(currentDate.y), std::to_string(currentDate.z))).c_str());

	ImGui::Checkbox("Use Ephemeris Positions", &useEphemerisPos);
	if (simulate && !useEphemerisPos) useEphemerisPos = true;

	ImGui::Text("Sun Position: ");
	ImGui::DragFloat("Sun Altitude", &sunAltAzi.x, 1.0f, -90.0f, 90.0f);
	ImGui::DragFloat("Sun Azimuth", &sunAltAzi.y, 1.0f, 0.0f, 360.0f);

	ImGui::Text("Moon Position: ");
	ImGui::DragFloat("Moon Altitude", &moonAltAzi.x, 1.0f, -90.0f, 90.0f);
	ImGui::DragFloat("Moon Azimuth", &moonAltAzi.y, 1.0f, 0.0f, 360.0f);



	ImGui::DragFloat3("Observer Pos", glm::value_ptr(cam.position), 0.1f);
	ImGui::DragFloat3("View Direction", glm::value_ptr(cam.direction), 0.1f);

	ImGui::End();
}

void imguiRender()
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}