#include <iostream>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"
using namespace std;

int main(int argc, char** argv) {
    cout << "Hello exercises04" << endl;

    glm::vec3 A = glm::vec3(2,6,-3);
    glm::vec3 B = glm::vec3(1,9,2);
    
    // from A to B
    glm::vec3 AtoB = B - A;
    cout << "A: " << glm::to_string(A) << endl;
    cout << "B: " << glm::to_string(B) << endl;
    cout << "AtoB: " << glm::to_string(AtoB) << endl;

    glm::vec3 color(1,1,0);
    glm::vec4 out_color = glm::vec4(color, 1.0);

    float lengthA = glm::length(A);
    glm::vec3 normA = glm::normalize(A);
    cout << "Length of A: " << lengthA << endl;
    cout << "Normalized A: " << glm::to_string(normA) << endl;


    return 0;
}