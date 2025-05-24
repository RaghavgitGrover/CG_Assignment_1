Instructions to run: 
make ; 
./sample

---

ImGui is incorporated to provide an on-screen informational panel. The steps are as follows:
ImGui context is created in the InitImGui() method. The code calls ImGui::CreateContext() and sets a modern dark style with ImGui::StyleColorsDark().
Functions ImGui_ImplGlfw_InitForOpenGL(window, true) and ImGui_ImplOpenGL3_Init("#version 330") initialize ImGui to work with our window and OpenGL version.
Each frame, the application starts a new ImGui frame with ImGui_ImplOpenGL3_NewFrame() and ImGui_ImplGlfw_NewFrame(), then creates a simple window that displays game time, remaining marbles, available controls, and any status messages.
After the game scene is rendered, ImGui’s draw data is rendered on top by calling ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData()).
