#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <CL/cl.h>
#include <GL/glut.h>
#include <stdio.h>

#define WIDTH 800
#define HEIGHT 600

const char* kernelSource =
"__kernel void render(__global float* output, const int width, const int height)\n"
"{\n"
"    int gid_x = get_global_id(0);\n"
"    int gid_y = get_global_id(1);\n"
"    int index = gid_y * width + gid_x;\n"
"    output[index] = sin(gid_x/10.0) * cos(gid_y/10.0);\n"
"}\n";

cl_platform_id platform;
cl_device_id device;
cl_context context;
cl_command_queue queue;
cl_program program;
cl_kernel kernel;

float* output;

void initializeCL() {
    clGetPlatformIDs(1, &platform, NULL);
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);

    context = clCreateContext(NULL, 1, &device, NULL, NULL, NULL);
    queue = clCreateCommandQueue(context, device, 0, NULL);

    program = clCreateProgramWithSource(context, 1, &kernelSource, NULL, NULL);
    clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    kernel = clCreateKernel(program, "render", NULL);

    output = (float*)malloc(WIDTH * HEIGHT * sizeof(float));
}

void renderCL() {
    cl_mem outputBuffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, WIDTH * HEIGHT * sizeof(float), NULL, NULL);

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &outputBuffer);
    clSetKernelArg(kernel, 1, sizeof(int), &WIDTH);
    clSetKernelArg(kernel, 2, sizeof(int), &HEIGHT);

    size_t globalWorkSize[] = { WIDTH, HEIGHT };
    clEnqueueNDRangeKernel(queue, kernel, 2, NULL, globalWorkSize, NULL, 0, NULL, NULL);
    clFinish(queue);

    clEnqueueReadBuffer(queue, outputBuffer, CL_TRUE, 0, WIDTH * HEIGHT * sizeof(float), output, 0, NULL, NULL);
    clReleaseMemObject(outputBuffer);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    glBegin(GL_POINTS);
    for (int i = 0; i < WIDTH; ++i) {
        for (int j = 0; j < HEIGHT; ++j) {
            float value = output[j * WIDTH + i];
            glColor3f(value, value, value);
            glVertex2f(i, j);
        }
    }
    glEnd();

    glFlush();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("OpenCL Visualization");

    initializeCL();
    renderCL();

    glMatrixMode(GL_PROJECTION);
    gluOrtho2D(0, WIDTH, 0, HEIGHT);
    glutDisplayFunc(display);
    glutMainLoop();

    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    free(output);

    return 0;
}
