#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <locale.h>
#include <getopt.h>
#include <string.h>
#include <math.h>
#include <wchar.h>

#define ERROR_OPEN_FILE 40
#define ERROR_FORMAT_FILE 41
#define ERROR_OUTPUT_FILE 42
#define ERROR_UNKNOWN_OPTION_OR_MIS_ARGUM 43
#define ERROR_SAME_INPUT_OUTPUT 44
#define ERROR_INVALID_FLAGS 45

#pragma pack(push, 1) // отключаем выравнивание

typedef struct { // определение структуры данных BitmapFileHeader
    unsigned short signature;
    unsigned int filesize;
    unsigned short reserved1;
    unsigned short reserved2;
    unsigned int fileOffsetToPixelArray;
} BitmapFileHeader; // просто предоставляет инфу о файле формата BMP. 

typedef struct { // определение структуры BitmapInfoHeader
    unsigned int headerSize;
    unsigned int imageWidth;
    unsigned int imageHeight;
    unsigned short planes;
    unsigned short bitsPerPixel;
    unsigned int compression;
    unsigned int imageSize;
    unsigned int xPixelsPerMeter;
    unsigned int yPixelsPerMeter;
    unsigned int totalColors;
    unsigned int importantColorCount;
} BitmapInfoHeader;

typedef struct { // структура для rgb
    unsigned char r;
    unsigned char g;
    unsigned char b;
} Rgb;

#pragma pack(pop) // включаем выравнивание

typedef struct { // структура для линии
    long int start[2]; // Массив из двух целых чисел, представляющий координаты начальной точки линии.
    long int end[2]; // Массив из двух целых чисел, представляющий координаты конечной точки линии.
    long int color[3]; // Массив из трех целых чисел, представляющий цвет линии в формате RGB.
    long int thickness; // толщина
} Line;

typedef struct { // структура для инвентированного круга
    long int center[2]; // Массив из двух целых чисел, который представляет координаты центра круга.
    long int radius; // Целое число, которое, вероятно, обозначает радиус круга.
} InverseCircle;

typedef struct { // структура для обрезочки
    long int leftUp[2]; // Массив из двух целых чисел, который представляет координаты левого верхнего угла области обрезки.
    long int rightDown[2]; // Массив из двух целых чисел, который представляет координаты правого нижнего угла области обрезки.
} Trim;

void printHelp() {
    printf("Help: Программа поддерживает беспалитровые bmp файлы с глубиной 24 бит на пиксель.\n"); // заполнить команду принтхелп
}

void printFileHeader(BitmapFileHeader bmfh) { // выводит соответсвующую информацию структуры BitmapFileHeader
    printf("signature:\t%x (%hu)\n", bmfh.signature, bmfh.signature);
    printf("filesize:\t%x (%u)\n", bmfh.filesize, bmfh.filesize);
    printf("reserved1:\t%x (%hu)\n", bmfh.reserved1, bmfh.reserved1);
    printf("reserved2:\t%x (%hu)\n", bmfh.reserved2, bmfh.reserved2);
    printf("fileOffsetToPixelArray:\t%x (%u)\n", bmfh.fileOffsetToPixelArray, bmfh.fileOffsetToPixelArray);
}

void printInfoHeader(BitmapInfoHeader bmif) { // выводит соответствующую инфформацию по файлу bmp (структура BitmapInfoHeader)
    printf("headerSize:\t%x (%u)\n", bmif.headerSize, bmif.headerSize);
    printf("imageWidth:     \t%x (%u)\n", bmif.imageWidth, bmif.imageWidth);
    printf("imageHeight:    \t%x (%u)\n", bmif.imageHeight, bmif.imageHeight);
    printf("planes:    \t%x (%hu)\n", bmif.planes, bmif.planes);
    printf("bitsPerPixel:\t%x (%hu)\n", bmif.bitsPerPixel, bmif.bitsPerPixel);
    printf("compression:\t%x (%u)\n", bmif.compression, bmif.compression);
    printf("imageSize:\t%x (%u)\n", bmif.imageSize, bmif.imageSize);
    printf("xPixelsPerMeter:\t%x (%u)\n", bmif.xPixelsPerMeter, bmif.xPixelsPerMeter);
    printf("yPixelsPerMeter:\t%x (%u)\n", bmif.yPixelsPerMeter, bmif.yPixelsPerMeter);
    printf("totalColors:\t%x (%u)\n", bmif.totalColors, bmif.totalColors);
    printf("importantColorCount:\t%x (%u)\n", bmif.importantColorCount, bmif.importantColorCount);
}


Rgb** readBmp(char file_name[], BitmapFileHeader* bmfh, BitmapInfoHeader* bmif) {
    FILE* file = fopen(file_name, "rb"); // открытие файла в режиме двоичного чтения
    if (!file) {
        printf("Ошибка: Входной файл  не найден.\n");
        exit(ERROR_OPEN_FILE);
    }

    fread(bmfh, 1, sizeof(BitmapFileHeader), file); //указатель на структуру, 1 эл, размер, файл
    if (bmfh->signature != 0x4d42) { // проверка на сигнатуру bmp
        printf("Ошибка: Данная версия не поддерживается.\n");
        exit(ERROR_FORMAT_FILE);
    }

    fread(bmif, 1, sizeof(BitmapInfoHeader), file); //проверка инфохедера
    //if (bmif->headerSize != 40) {
        //printf("Ошибка: Файл имеет непподерживаемый формат.\n");
        //exit(ERROR_FORMAT_FILE);
    // } почему то лагает сэ тим эээ блин
    if (bmif->bitsPerPixel != 24) {
        printf("Ошибка: На вход принимаются только 24 битные изображения.\n");
        exit(ERROR_FORMAT_FILE);
    }
    if (bmif->compression != 0) {
        printf("Ошибка: Изображение не должно быть сжатым.\n");
        exit(ERROR_FORMAT_FILE);
    }

    unsigned int height = bmif->imageHeight;//берет высоту и ширину
    unsigned int width = bmif->imageWidth;
    if (height > 65535 || width > 65535) {
        printf("Ошибка: Изображение слишком большое.\n");
        exit(ERROR_FORMAT_FILE);
    }

    Rgb** arr = malloc(height * sizeof(Rgb*)); //количество пикселей
    for (int i = 0; i < height; i++) { // по каждой высоте 
        arr[i] = malloc(width * sizeof(Rgb) + ((4 - (width * sizeof(Rgb)) % 4) % 4)); //bmp требует чтобы строка была кратна 4 байтам
        fread(arr[i], 1, width * sizeof(Rgb) + ((4 - (width * sizeof(Rgb)) % 4) % 4), file);
    }
    fclose(file);
    return arr;
}


void writeBmp(char file_name[], Rgb** arr, int height, int width, BitmapFileHeader bmfh, BitmapInfoHeader bmif) {
    FILE* file = fopen(file_name, "wb");
    if (!file) {
        printf("Ошибка: Выходной файл не найден.\n");
        exit(ERROR_OUTPUT_FILE);
    }

    fwrite(&bmfh, sizeof(BitmapFileHeader), 1, file);
    fwrite(&bmif, sizeof(BitmapInfoHeader), 1, file);
    for (int i = 0; i < height; i++) {
        fwrite(arr[i], 1, width * sizeof(Rgb) + ((4 - (width * sizeof(Rgb)) % 4) % 4), file);
    }
    fclose(file);
}


void setPixelColor(Rgb** arr, int x, int y, long int* color) { // изменяет цвет пикселя в заданном местоположении двумерного массива
    arr[y][x].r = color[2];
    arr[y][x].g = color[1];
    arr[y][x].b = color[0];
}


// Алгоритм Брезенхэма. Идея заключается в итеративном обновлении координат x и y с учетом накопления ошибки.
void drawLine(Rgb** image, unsigned int height, unsigned int width, long int start[2], long int end[2], long int color[3]) {
    int x = start[0];
    int y = start[1];

    const int deltaX = abs(end[0] - start[0]); // Вычисляем разность по x и по y
    const int deltaY = abs(end[1] - start[1]);
    int error = deltaX - deltaY; // Вычисляем начальное значение ошибки

    const int signX = start[0] < end[0] ? 1 : -1;
    const int signY = start[1] < end[1] ? 1 : -1;

    while ((x != end[0]) || (y != end[1])) { // Проходим по всем пикселям на линии
        if ((x < width) && (y < height)) {
            setPixelColor(image, x, y, color);
        }
        // Сравнение error2 с -deltaY и deltaX определяет, какой пиксель на линии нужно закрасить следующим шагом.
        int error2 = error * 2; // Вычисляем ошибку дважды, чтобы учесть движение по x и по y
        if (error2 > -deltaY) {
            error -= deltaY;
            x += signX;
        }
        if (error2 < deltaX) {
            error += deltaX;
            y += signY;
        } // В целом, этот фрагмент кода является важной частью алгоритма Брезенхема для рисования линий. 
    } // Он оптимизирует расчет шагов по x и y, обеспечивая более эффективное и точное рисование линий.
}


void drawCircle(Rgb** image, unsigned int height, unsigned int width, int cenX, int cenY, long int* color, long int radius) { // рисует окружность
    for (int x = -radius; x < radius; x++) { // проходим по всем значениям круга
        int min_y = round(sqrt(radius * radius - x * x)); // значение y которое соответсвтует x на этой окружности
        // Перед установкой цвета пикселя проверяется, находится ли он в пределах изображения.
        if ((x + cenX >= width) || (x + cenX < 0)) { // если выходит за границы изображения, пропускаем
            continue;
        }
        for (int y = -min_y; y < min_y; y++) {
            if ((y + cenY >= height) || (y + cenX < 0)) {
                continue;
            }
            setPixelColor(image, x + cenX, y + cenY, color);
        }
    }
}


void writeLine(Rgb** image, unsigned int height, unsigned int width, long int startX, long int startY, long int endX, long int endY, long int color[3], long int thickness) {
    int radius = round(thickness / 2);

    int dif[] = { startX - endX, startY - endY }; // В него записываются значения разницы между координатами конечной
    float delta[2]; // Он предназначен для хранения нормированного вектора направления линии.
    int max_delta = 0; // Она будет хранить абсолютную величину большей разницы по X или Y.
    drawCircle(image, height, width, startX, startY, color, radius); // начала рисует границу

    if (abs(dif[0]) >= abs(dif[1])) { //  какая из разностей по X или Y является большей по абсолютной величине.
        max_delta = abs(dif[0]);
    }
    else {
        max_delta = abs(dif[1]);
    }

    if (max_delta != 0) { // Это означает, что линия, которую нужно рисовать, имеет ненулевую длину
        drawCircle(image, height, width, endX, endY, color, radius); // может быть использована для визуального выделения конца
        delta[0] = dif[0] / max_delta; // масштабирует разницу координат до единичного вектора, который указывает направление линии.
        delta[1] = dif[1] / max_delta;

        delta[0] = delta[0] / (sqrt(delta[0] * delta[0] + delta[1] * delta[1])); // Нормализация вектора направления 
        delta[1] = delta[1] / (sqrt(delta[0] * delta[0] + delta[1] * delta[1]));

        radius = radius * 1.1; // умножается на 1.1, делая наконечники стрелок немного больше толщины линии.
        // Создаются два наконечника стрелок
        long int startFirst[] = { round(startX - radius * delta[1]), round(startY + radius * delta[0]) - 1 }; // startFirst хранят начальные координаты для первого наконечника
        long int endFirst[] = { round(endX - radius * delta[1]), round(endY + radius * delta[0]) - 1 }; // endFirst хранят  конечные координаты для первого наконечника
        long int startSecond[] = { round(startX + radius * delta[1]), round(startY - radius * delta[0]) - 1 };
        long int endSecond[] = { round(endX + radius * delta[1]), round(endY - radius * delta[0]) - 1 };
        drawLine(image, height, width, startFirst, endFirst, color);

        const int deltaX = abs(startSecond[0] - startFirst[0]);
        const int deltaY = abs(startSecond[1] - startFirst[1]);

        const int signX = startFirst[0] < startSecond[0] ? 1 : -1;
        const int signY = startFirst[1] < startSecond[1] ? 1 : -1;

        int error = deltaX - deltaY;
        int x = startFirst[0];
        int y = startFirst[1];

        while ((x != startSecond[0]) || (y != startSecond[1])) {
            long int start[] = { x, y };
            long int end[] = { x + dif[0], y + dif[1] };
            drawLine(image, height, width, start, end, color);

            if ((abs(delta[0]) <= 0.6) && ((start[0] < startSecond[0] - 1) || (start[0] < startFirst[0]))) {
                start[0] += 1;
                end[0] += 1;
            }
            else if ((abs(delta[1]) < 0.6) && ((start[1] < startSecond[1] - 1) || (start[1] < startFirst[1]))) {
                start[1] += 1;
                end[1] += 1;
            }
            drawLine(image, height, width, start, end, color);

            int error2 = error * 2;
            if (error2 > -deltaY) {
                error -= deltaY;
                x += signX;
            }
            if (error2 < deltaX) {
                error += deltaX;
                y += signY;
            }
        }
    }
}


void invertColors(Rgb** image, unsigned int height, unsigned int width, long int center[2], long int radius) { //инвертирование цветов пикселей внутри круга на изображении.
    long int color[3]; // Массив целых чисел размером 3, который используется для хранения инвертированных значений цвета
    const unsigned char maxСolorValue = 255;
    for (int i = -radius; i <= radius; i++) { //итерация по y
        for (int j = -radius; j <= radius; j++) { //итерация по x
            int x = center[0] + j;
            int y = center[1] + i;
            int sq_dist = i * i + j * j;
            if ((sq_dist <= radius * radius) && (x < width) && (y < height) && (x >= 0) && (y >= 0)) {
                color[0] = maxСolorValue - image[x][y].b;
                color[1] = maxСolorValue - image[x][y].g;
                color[2] = maxСolorValue - image[x][y].r;
                setPixelColor(image, x, y, color);
            }
        }
    }
}


void trim_image(Rgb*** image, BitmapFileHeader** bmfh, BitmapInfoHeader** bmif, unsigned int H, unsigned int W, long int LU[2], long int RD[2]) {
    unsigned int nH = LU[1] - RD[1];
    unsigned int nW = RD[0] - LU[0];

    if ((nH == 0) && (nW == 0)) {
        nH = 1;
        nW = 1;
        Rgb** arr = malloc(1 * sizeof(Rgb));

        arr[0] = malloc(nW * sizeof(Rgb) + ((4 - (nW * sizeof(Rgb)) % 4) % 4));
        arr[0][0] = (*image)[RD[1]][LU[0]];

        *image = arr;
    }
    else if ((nH == 0) && (nW != 0)) {
        nH = 1;

        Rgb** arr = malloc(1 * sizeof(Rgb*));
        arr[0] = malloc(nW * sizeof(Rgb) + ((4 - (nW * sizeof(Rgb)) % 4) % 4));

        for (int j = 0; j < nW; j++) {
            arr[0][j] = (*image)[RD[1]][LU[0] + j];
        }

        *image = arr;
    }
    else if ((nH != 0) && (nW == 0)) {
        nW = 1;

        Rgb** arr = malloc(nH * sizeof(Rgb*));

        for (int i = 0; i < nH; i++) {
            arr[i] = malloc(nW * sizeof(Rgb) + ((4 - (nW * sizeof(Rgb)) % 4) % 4));
            arr[i][0] = (*image)[RD[1] + i][LU[0]];
        }

        *image = arr;
    }
    else {
        Rgb** arr = malloc(nH * sizeof(Rgb*));

        for (int i = 0; i < nH; i++) {
            arr[i] = malloc(nW * sizeof(Rgb) + ((4 - (nW * sizeof(Rgb)) % 4) % 4));
            for (int j = 0; j < nW; j++) {
                arr[i][j] = (*image)[RD[1] + i][LU[0] + j];
            }
        }

        for (int i = 0; i < H; i++) {
            free((*image)[i]);
        }

        *image = arr;
    }

    (*bmif)->imageHeight = nH;
    (*bmif)->imageWidth = nW;
}


void check_flag(int flag, int num) {
    if (flag < num) {
        printf("Ошибка: Недопустимое количество флагов.\n");
        exit(ERROR_INVALID_FLAGS);
    }
}


int main(int argc, char** argv) {
    printf("Course work for option 4.6, created by Sychev Nickolay.\n");
    setlocale(LC_ALL, "");

    BitmapInfoHeader* bmif = malloc(sizeof(BitmapInfoHeader));
    BitmapFileHeader* bmfh = malloc(sizeof(BitmapFileHeader));
    Line line;
    InverseCircle inverse_circle;
    Trim trim;

    int lin = 0;
    int inv_c = 0;
    int trm = 0;
    int info_mark = 0;
    int c = 0;
    optind = 0;
    opterr = 0;
    char* inpstr;
    char* filename = malloc(sizeof(char) * (100 + 1));
    char* output_file = malloc(sizeof(char) * (100 + 1));
    int i = 0;
    int mark_f_inp = 0;
    int mark_f_out = 0;

    struct option long_options1[] = {
        {"line", no_argument, 0, 'l'},
        {"start", required_argument, 0, 's'},
        {"end", required_argument, 0, 'e'},
        {"color", required_argument, 0, 'c'},
        {"thickness", required_argument, 0, 't'},
        {"inverse_circle", no_argument, 0, 'v'},
        {"center", required_argument, 0, 'C'},
        {"radius", required_argument, 0, 'r'},
        {"trim", no_argument, 0, 'T'},
        {"left_up", required_argument, 0, 'L'},
        {"right_down", required_argument, 0, 'R'},
        {"input", required_argument, 0, 'i'},
        {"output", required_argument, 0, 'o'},
        {"info", no_argument, 0, 'I'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    while ((c = getopt_long(argc, argv, "ls:e:c:t:vC:r:TL:R:i:o:Ih", long_options1, &optind)) != -1) {
        switch (c) {
        case 'l':
            lin += 1;
            break;
        case 'v':
            inv_c += 1;
            break;
        case 'T':
            trm += 1;
            break;
        case 's':
            lin += 1;
            if (strstr(optarg, ".") == NULL) { //проверка наличия значения для аргумента
                printf("Invalid input value.\n");
                return 42;
            }
            line.start[0] = strtol(strtok(optarg, "."), NULL, 10);
            line.start[1] = strtol(strtok(NULL, "."), NULL, 10);
            break;
        case 'e':
            lin += 1;
            if (strstr(optarg, ".") == NULL) { //проверка наличия значения для аргумента
                printf("Invalid input value.\n");
                return 42;
            }
            line.end[0] = strtol(strtok(optarg, "."), NULL, 10);
            line.end[1] = strtol(strtok(NULL, "."), NULL, 10);
            break;
        case 'c':
            lin += 1;
            if (strstr(optarg, ".") == NULL) {
                printf("Invalid input value.\n");
                return 42;
            }
            inpstr = strtok(optarg, ".");
            int count = 1;
            while (inpstr != NULL) {
                if (strtol(inpstr, NULL, 10) < 0 || strtol(inpstr, NULL, 10) > 255 || (strtol(inpstr, NULL, 10) == 0 && strcmp(inpstr, "0") != 0)) {
                    printf("Invalid input value.\n");
                    return 42;
                }
                line.color[i] = strtol(inpstr, NULL, 10);
                i += 1;
                inpstr = strtok(NULL, ".");
                count += 1;
                if (inpstr == NULL && count != 4) {
                    printf("Invalid input value.\n");
                    return 42;
                }
            }
            break;
        case 't':
            lin += 1;
            if (strtol(optarg, NULL, 10) < 0 || strcmp(optarg, "0") == 0) {
                printf("Invalid input value.\n");
                return 42;
            }
            line.thickness = strtol(optarg, NULL, 10);
            break;
        case 'C':
            inv_c += 1;
            if (strstr(optarg, ".") == NULL) {
                printf("Invalid input value.\n");
                return 42;
            }
            inverse_circle.center[0] = strtol(strtok(optarg, "."), NULL, 10);
            inverse_circle.center[1] = strtol(strtok(NULL, "."), NULL, 10);
            break;
        case 'r':
            inv_c += 1;
            inverse_circle.radius = strtol(optarg, NULL, 10);
            break;
        case 'L':
            trm += 1;
            if (strstr(optarg, ".") == NULL) {
                printf("Invalid input value.\n");
                return 42;
            }
            trim.leftUp[0] = strtol(strtok(optarg, "."), NULL, 10);
            trim.leftUp[1] = strtol(strtok(NULL, "."), NULL, 10);
            break;
        case 'R':
            trm += 1;
            if (strstr(optarg, ".") == NULL) {
                printf("Invalid input value.\n");
                return 42;
            }
            trim.rightDown[0] = strtol(strtok(optarg, "."), NULL, 10);
            trim.rightDown[1] = strtol(strtok(NULL, "."), NULL, 10);
            break;
        case 'i':
            filename = realloc(filename, (strlen(optarg) + 1) * sizeof(char));
            strcpy(filename, optarg);
            mark_f_inp = 1;
            break;
        case 'o':
            output_file = realloc(output_file, sizeof(char) * (strlen(optarg) + 1));
            strcpy(output_file, optarg);
            mark_f_out = 1;
            break;
        case 'I':
            info_mark = 1;
            break;
        case 'h':
            printHelp();
            return 0;
        default:
            printf("Unknown option or missing argument.\n");
            return ERROR_UNKNOWN_OPTION_OR_MIS_ARGUM;
        }
    }

    if ((mark_f_inp == 0) || (mark_f_out == 0)) {
        if ((mark_f_inp == 0) && (mark_f_out != 0)) {
            filename = realloc(filename, (strlen(argv[optind]) + 1) * sizeof(char));
            strcpy(filename, argv[optind]);
        }
        else if ((mark_f_inp != 0) && (mark_f_out == 0)) {
            output_file = realloc(output_file, (7 + 1) * sizeof(char));
            strcpy(output_file, "out.bmp");
        }
        else {
            filename = realloc(filename, (strlen(argv[optind]) + 1) * sizeof(char));
            output_file = realloc(output_file, (7 + 1) * sizeof(char));
            strcpy(output_file, "out.bmp");
            strcpy(filename, argv[optind]);
        }
        if (strcmp(filename, output_file) == 0) {
            printf("Ошибка: Одинакове название входного и выходного файлов.\n");
            return ERROR_SAME_INPUT_OUTPUT;
        }
    }

    Rgb** image = readBmp(filename, bmfh, bmif); //передали указатель на первый байт в памяти

    if (lin > 0) {
        check_flag(lin, 5);
        writeLine(image, bmif->imageHeight, bmif->imageWidth, line.start[0], line.start[1], line.end[0], line.end[1], line.color, line.thickness);
    }
    else if (inv_c > 0) {
        check_flag(inv_c, 3);
        invertColors(image, bmif->imageHeight, bmif->imageWidth, inverse_circle.center, inverse_circle.radius);
    }
    else if (trm > 0) {
        check_flag(trm, 3);

        if (trim.leftUp[0] < 0) {
            trim.leftUp[0] = 0;
        }
        if (trim.leftUp[1] < 0) {
            trim.leftUp[1] = 0;
        }
        if (trim.rightDown[1] < 0) {
            trim.rightDown[1] = 0;
        }
        if (trim.rightDown[0] < 0) {
            trim.rightDown[0] = 0;
        }
        if (trim.leftUp[1] >= bmif->imageHeight) {
            trim.leftUp[1] = bmif->imageHeight - 1;
        }
        if (trim.leftUp[0] >= bmif->imageWidth) {
            trim.leftUp[0] = bmif->imageWidth - 1;
        }
        if (trim.rightDown[0] >= bmif->imageWidth) {
            trim.rightDown[0] = bmif->imageWidth - 1;
        }
        if (trim.rightDown[1] >= bmif->imageHeight) {
            trim.rightDown[1] = bmif->imageHeight - 1;
        }

        if (trim.leftUp[0] > trim.rightDown[0]) {
            long int tmp = trim.rightDown[0];
            trim.rightDown[0] = trim.leftUp[0];
            trim.leftUp[0] = tmp;
        }
        if (trim.leftUp[1] < trim.rightDown[1]) {
            long int tmp = trim.rightDown[1];
            trim.rightDown[1] = trim.leftUp[1];
            trim.leftUp[1] = tmp;
        }

        trim_image(&image, &bmfh, &bmif, bmif->imageHeight, bmif->imageWidth, trim.leftUp, trim.rightDown);
    }
    else if (info_mark > 0) {
        printFileHeader(*bmfh);
        printInfoHeader(*bmif);
    }

    writeBmp(output_file, image, bmif->imageHeight, bmif->imageWidth, *bmfh, *bmif);

    free(filename);
    free(output_file);
    for (unsigned int k = 0; k < bmif->imageHeight - 1; k++) {
        free(image[k]);
    }
    free(image);
    free(bmfh);
    free(bmif);

    return 0;
}