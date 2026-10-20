# Лабораторная №1. Равномерное распределение точек

C++Builder + VCL + GLScene. Генерация 100 000 точек или сфер в кубе, сфере,
цилиндре и конусе. Камера снаружи и в центре, оси XYZ, случайное время свечения
1–100 секунд и измерение FPS.

## Примеры работы

100 000 точек в кубе.

![Точки в кубе](images/Cube-Points-Outside.png)

Сферы случайного радиуса и семи цветов в сферическом контейнере.

![Сферы в сфере](images/Sphere-Spheres-Outside.png)

Равномерное распределение точек в цилиндре.

![Точки в цилиндре](images/Cylinder-Points-Outside.png)

Сферы в коническом контейнере.

![Сферы в конусе](images/Cone-Spheres-Outside.png)

## Сборка

Нужны RAD Studio с C++Builder для Win32 и [GLXEngine (GLScene)](https://github.com/glscene/GLXEngine).
Использована версия GLXEngine `aecc61b20c570f2daaa94e886576e9e5333d85a4`.

1. Собрать `Packages/GLScene_RT.dproj` с заголовками и библиотеками C++.
2. Открыть `RandomStars_cpp.cbproj`, выбрать Release / Win32.
3. Указать `GLSCENE_ROOT` (корень GLXEngine) и `WINSDK_ROOT` (подготовленные заголовки Windows SDK).
   Папки `GLXEngine` и `WindowsSDK/37.0` рядом с папкой проекта определяются автоматически.
4. Собрать и запустить; `GLScene_RT.bpl` и runtime-пакеты RAD Studio должны быть доступны приложению.

Также можно запустить `build.cmd` из RAD Studio Command Prompt.

## Проверка генератора

Ядро генератора не зависит от GLScene. Для проверки нужен компилятор C++11 и CMake:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Примеры для изучения: [AstrobloQ](https://github.com/glscene/AstrobloQ).
Библиотека GLScene устанавливается отдельно.
