#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "shader.h"
#include "camera.h"

#include <iostream>
#include <numbers>

#include "gtc/type_ptr.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

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
float sunAngle{160.0f};
float moonAngle{ 50.0f };
float sunScale{ 10.0f };
bool limbDarken{ true };
float moonIntensity{ 888888.0f };
float moonRotAngle{ 0.0f };

bool animateSun{ true };
float animSpeed{ 2.0f };

float totalSimHours{};

float solarTime{ 0.0f };
int dayOfTheYear{ 50};
glm::vec2 latLong{ 40.0f, -3.7f };
glm::vec2 sunAltAzi{0.0f, 270.0f};
glm::vec2 moonAltAzi{};

glm::vec3 date{ 21, 6, 2026 };
Camera cam;

double julianDay(double d, int m, int y)
{
	if (m <= 2)
	{
		m = m + 12;
		y = y - 1;
	}

	// Using gregorian calender, B = 0 if julian 
	const int A = floor(y / 100.0f);
	const int B = 2 - A + floor(A / 4.0f);
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

void doyToDate(int& doy, int year, int& day, int& month, int& outYear)
{
	outYear = year;

	while (true)
	{
		int daysInYear = isLeap(outYear) ? 366 : 365;
		if (doy <= daysInYear) break;
		doy -= daysInYear;
		outYear++;
	}

	int daysPerMonth[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
	if (isLeap(outYear)) daysPerMonth[1] = 29;

	month = 0;
	int fulldays = doy;
	while (fulldays > daysPerMonth[month])
	{
		fulldays -= daysPerMonth[month];
		month++;
	}

	day = fulldays;
	month += 1;
}


double wrap(double a, double min, double max)
{
	const double range = max - min;
	a = fmod(a - min, range);
	if (a < 0.0) a += range;
	return a + min;
}

double wrap360(double deg) { return wrap(deg, 0.0, 360.0); }
double wrap180(double deg) { return wrap(deg, -180.0, 180.0); }
double wrap2pi(double rad) { return wrap(rad, 0.0, 2.0 * PI); }
double wrapPi(double rad) { return wrap(rad, -PI, PI); }


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

	double sumL = 6.289 * sin(glm::radians(Mp)) * E
				+ 1.274 * sin(glm::radians(2 * D + (-Mp))) * E
				+ 0.658 * sin(glm::radians(2 * D))
				+ 0.214 * sin(glm::radians(2 * Mp)) * (E * E)
				- 0.185 * sin(glm::radians(M)) * E
				- 0.114 * sin(glm::radians(2 * F));

	double sumB = 5.128 * sin(glm::radians(F))
				+ 0.280 * sin(glm::radians(Mp + F)) * E
				+ 0.277 * sin(glm::radians(Mp - F)) * E
				+ 0.173 * sin(glm::radians(2 * D - F));

	// Ecliptic longitude
	double lambda = ((wrap(glm::radians(Lp + sumL), 0.0f, TWO_PI)));

	// Ecliptic latitude
	double beta = glm::radians(sumB);


	double eps = glm::radians(23.0 + 26.0 / 60.0 + 21.448 / 3600.0 - (46.8150 * t + 0.00059 * t * t - 0.001813 * t * t * t) / 3600.0f);

	double x = cos(beta) * cos(lambda);
	double y = cos(eps) * cos(beta) * sin(lambda) - sin(eps) * sin(beta);
	double z = sin(eps) * cos(beta) * sin(lambda) + cos(eps) * sin(beta);
	double r = sqrt(1 - (z * z));

	double ra = atan2(y, x);
	double dec = atan2(z, sqrt(x * x + y * y)); 

	ra = wrap(ra, 0.0, TWO_PI);

	double theta0_deg = 280.46061837
		+ 360.98564736629 * (jd - 2451545.0)
		+ 0.000387933 * t * t
		- (t * t * t) / 38710000.0;

	theta0_deg = wrap360(theta0_deg);
	double theta0 = glm::radians(theta0_deg);
	double theta = theta0 + longitude;  // longitude already in radians
	double hourAngle = wrapPi(theta - ra);



	float azimuth = (float)atan2((sin(hourAngle)), (sin(lat) * cos(hourAngle) - tan(dec) * cos(lat)));

	double sinAlt = sin(lat) * sin(dec) + cos(lat) * cos(dec) * cos(hourAngle);

	sinAlt = glm::clamp(sinAlt, -1.0, 1.0);
	float altitude = (float)asin(sinAlt);
	azimuth = wrap(azimuth, 0.0, TWO_PI);
	moonAltAzi.x = glm::degrees(altitude);
	moonAltAzi.y = glm::degrees(azimuth);

	glm::vec3 moonDir = glm::vec3(glm::cos(altitude) * glm::sin(azimuth),
		glm::sin(altitude),
		glm::cos(altitude) * glm::cos(azimuth));



	if (fabs(moonDir.x) < 1e-12f) moonDir.x = 0.0f;
	if (fabs(moonDir.y) < 1e-12f) moonDir.y = 0.0f;
	if (fabs(moonDir.z) < 1e-12f) moonDir.z = 0.0f;


	return glm::normalize(moonDir);
}


glm::vec3 sunDirectionFromTime(float time, int day, float degLatitude)
{

	float lat = glm::radians(degLatitude);
	float hourAngle = glm::radians(15.0f * (time - 12.0f));
	
	// Day angle
	const float g = 2 * PI * (float(day - 1) / 365.0f);

	// declination, spencer's formula
	double dec = 0.006918 - 0.399912 * cos(g) + 0.070257 * sin(g) - 0.006758 * cos(2 * g) + 0.000907 * sin(2 * g) - 0.002697 * cos(3 * g) + 0.001480 * sin(3 * g);


	double sinAlt = sin(lat) * sin(dec) + cos(lat) * cos(dec) * cos(hourAngle);
	sinAlt = glm::clamp(sinAlt, -1.0, 1.0);
	float altitude = (float)asin(sinAlt);

	float azimuth = (float)atan2((sin(hourAngle)), (sin(lat) * cos(hourAngle) - tan(dec) * cos(lat)));

	azimuth = wrap(azimuth, 0.0f, TWO_PI);
	sunAltAzi.x = glm::degrees(altitude);
	sunAltAzi.y = glm::degrees(azimuth);



	glm::vec3 sunDir = glm::vec3(glm::cos(altitude) * glm::sin(azimuth),
		glm::sin(altitude),
		glm::cos(altitude) * glm::cos(azimuth));

	if (fabs(sunDir.x) < 1e-12f) sunDir.x = 0.0f;
	if (fabs(sunDir.y) < 1e-12f) sunDir.y = 0.0f;
	if (fabs(sunDir.z) < 1e-12f) sunDir.z = 0.0f;


	return glm::normalize(sunDir);

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
	
	cam.position = glm::vec3(0.0f, 6360e3 + 1, 0.0f);
	cam.direction = glm::vec3(-1.0f, 0.0f, 0.0f);
	cam.aspect = (float)SCR_WIDTH / SCR_HEIGHT;

	Shader baseShader{ "shaders/base.vert", "shaders/sky.frag" };
	baseShader.use();
	baseShader.setUniformi("u_ScrWidth", SCR_WIDTH);
	baseShader.setUniformi("u_ScrHeight", SCR_HEIGHT);
	baseShader.setUniformBlock("SharedInfo", 0);

	imguiInit(window);
	glm::mat4 model{ 1.0f };
	model = glm::rotate(model, glm::radians(float(50.0f * std::sin(glfwGetTime()))), glm::vec3(0.0f, 0.0f, 1.0f));
	float lastFrameTime{};
	while (!glfwWindowShouldClose(window))
	{

		float currentTime = glfwGetTime();
		float deltaTime = currentTime - lastFrameTime;
		lastFrameTime = currentTime;
		glfwPollEvents();
		imguiUse();

	//mouseInput(cam, 0.05f, xOffset, yOffset);

		totalSimHours += animSpeed * deltaTime;

		solarTime = fmod(totalSimHours, 24.0f);

		float ut = fmod(totalSimHours - latLong.y / 15.0f, 24.0f);
		double jdStart = julianDay(21.0, 6, 2026);   
		double jd = jdStart + totalSimHours / 24.0;

		int day = date.x;
		int month = date.y;
		int year = date.z;
		doyToDate(dayOfTheYear, date.z, day, month, year);




		//std::cout << "Cam Direction: " << cam.direction.x << ", " << cam.direction.y << ", " << cam.direction.z << std::endl;
		//std::cout << "X Offset: " << xOffset << ", " << "Y Offset: " << yOffset << std::endl;

		glClearColor(0.53f, 0.0f, 0.42f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);


		float r = (6360e3 + 1.0f);
		cam.position = glm::vec3(r * glm::cos(glm::radians(latLong.x)) * glm::sin(glm::radians(latLong.y)), r * glm::sin(glm::radians(latLong.x)), r * glm::cos(glm::radians(latLong.x)) * glm::cos(glm::radians(latLong.y)));

		const float eps = r * 5e-8f;
		if (fabs(cam.position.x) < eps) cam.position.x = 0.0f;
		if (fabs(cam.position.y) < eps) cam.position.y = 0.0f;
		if (fabs(cam.position.z) < eps) cam.position.z = 0.0f;

		glm::vec3 worldUp = glm::normalize(cam.position);

		if (abs(glm::dot(cam.direction, worldUp)) > 0.999f)
		{
			worldUp = glm::vec3(1.0f, 0.0f, 0.0f);
			if (abs(glm::dot(cam.direction, worldUp)) > 0.999f)
				worldUp = glm::vec3(0.0f, 0.0f, 1.0f);
		}

		cam.up = worldUp;

		glm::vec3 sunDir = glm::vec3(glm::cos(glm::radians(sunAltAzi.x)) * glm::sin(glm::radians(sunAltAzi.y)),
		                             glm::sin(glm::radians(sunAltAzi.x)),
		                             glm::cos(glm::radians(sunAltAzi.x)) * glm::cos(glm::radians(sunAltAzi.y)));

		if (fabs(sunDir.x) < 1e-12f) sunDir.x = 0.0f;
		if (fabs(sunDir.y) < 1e-12f) sunDir.y = 0.0f;
		if (fabs(sunDir.z) < 1e-12f) sunDir.z = 0.0f;

		sunDir = glm::normalize(sunDir);

		sunDir = sunDirectionFromTime(solarTime, dayOfTheYear, latLong.x);

		glm::vec3 moonDir = glm::vec3(glm::cos(glm::radians(moonAltAzi.x)) * glm::sin(glm::radians(moonAltAzi.y)),
			glm::sin(glm::radians(moonAltAzi.x)),
			glm::cos(glm::radians(moonAltAzi.x)) * glm::cos(glm::radians(moonAltAzi.y)));

		if (fabs(moonDir.x) < 1e-12f) moonDir.x = 0.0f;
		if (fabs(moonDir.y) < 1e-12f) moonDir.y = 0.0f;
		if (fabs(moonDir.z) < 1e-12f) moonDir.z = 0.0f;

		moonDir = glm::normalize(moonDir);


		
		date = glm::vec3(21, 6, 2026);

		moonDir = moonDirectionFromTime(jd, latLong.x, latLong.y);

		date.x = day;
		date.y = month;
		date.z = year;

		baseShader.use();
		baseShader.setUniformMat4("u_InvProjection", glm::inverse(cam.getProjectionMat()));
		baseShader.setUniformMat4("u_InvView", glm::inverse(cam.getViewMat()));

		baseShader.setUniformf("u_MoonIntensity", moonIntensity);

		baseShader.setUniformi("u_LimbDarken", limbDarken);
		baseShader.setUniformf("u_SunScale", sunScale);

		baseShader.setUniformVec3("u_SunDir", sunDir);
		baseShader.setUniformVec3("u_MoonDir", moonDir);
		baseShader.setUniformVec3("u_Observer", cam.position);
		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(unsigned), GL_UNSIGNED_INT, nullptr);

		imguiRender();
		glfwSwapBuffers(window);
	}

	glfwTerminate();
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

	ImGui::DragFloat("Anim Speed", &animSpeed, 0.05f);

	ImGui::Text("Observer Positioning: ");
	ImGui::DragFloat("Latitude", &latLong.x, 0.1f, -90.0f, 90.0f);
	ImGui::DragFloat("Longitude", &latLong.y, 0.1f, -180.0f, 180.0f);

	ImGui::DragFloat("Solar Time", &solarTime, 0.1f, 0.0f, 24.0f);
	ImGui::DragInt("Day", &dayOfTheYear);

	ImGui::DragFloat3("Date", glm::value_ptr(date));

	ImGui::Text("Sun Positioning: ");
	ImGui::DragFloat("Sun Altitude", &sunAltAzi.x, 1.0f, -90.0f, 90.0f);
	ImGui::DragFloat("Sun Azimuth", &sunAltAzi.y, 1.0f, 0.0f, 360.0f);

	ImGui::Text("Moon Positioning: ");
	ImGui::DragFloat("Moon Altitude", &moonAltAzi.x, 1.0f, -90.0f, 90.0f);
	ImGui::DragFloat("Moon Azimuth", &moonAltAzi.y, 1.0f, 0.0f, 360.0f);

	ImGui::DragFloat("Moon Intensity", &moonIntensity);
	ImGui::DragFloat3("Observer Pos", glm::value_ptr(cam.position), 0.1f);
	ImGui::DragFloat3("ViewingDir", glm::value_ptr(cam.direction), 0.1f);
	ImGui::DragFloat("Sun Scale", &sunScale, 0.1f);
	ImGui::Checkbox("Darken Limb", &limbDarken);

	ImGui::End();
}

void imguiRender()
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}