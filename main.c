#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#define OPEN_INPUT_FILE 40 // +
#define OPEN_OUTPUT_FILE 41 // +
#define INVALID_FLAGS 42
#define INVALID_ARGUMENTS 43
#define UNKNOWN_OPTION_OR_MIS_ARGUM 44
#define SAME_INPUT_OUTPUT 45 

#pragma pack (push, 1) //+
typedef struct {
    unsigned short signature;
    unsigned int filesize;
    unsigned short reserved1;
    unsigned short reserved2;
    unsigned int pixelArrOffset;
} BitmapFileHeader; //+

typedef struct { // +
    unsigned int headerSize;
    unsigned int width;
    unsigned int height;
    unsigned short planes;
    unsigned short bitsPerPixel;
    unsigned int compression;
    unsigned int imageSize;
    unsigned int xPixelsPerMeter;
    unsigned int yPixelsPerMeter;
    unsigned int colorsInColorTable;
    unsigned int importantColorCount;
} BitmapInfoHeader; //+

typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} Pix; // +
#pragma pack(pop) // +

typedef struct {
    long int start[2];
    long int end[2];
    long int color[3];
    long int thick;
} Line; // +

typedef struct {
    long int center[2];
    long int rad;
} Inv_Cir; // +

typedef struct {
    long int leftUp[2];
    long int rightDown[2];
} Trim; // +

Pix** // +
read_bmp(char file_name[], BitmapFileHeader* bmfh, BitmapInfoHeader* bmif) {
    FILE *f = fopen(file_name, "rb");
    if (f == NULL) {
        printf("Error opening the input file.\n");
        exit(OPEN_INPUT_FILE);
    }
    fread(bmfh, 1, sizeof(BitmapFileHeader), f);
    if (bmfh->signature != 0x4D42) {
        printf("Error opening.\n");
        exit(OPEN_INPUT_FILE);
    }

    fread(bmif, 1, sizeof(BitmapInfoHeader), f);
    unsigned int H = bmif->height;
    unsigned int W = bmif->width;
    Pix **arr = malloc(H * sizeof(Pix*));
    for (int i = H - 1; i >= 0; i--){
        arr[i] = malloc(W * sizeof(Pix) + ((4 - (W * sizeof(Pix)) % 4) % 4));
        fread(arr[i], 1, W * sizeof(Pix) + ((4 - (W * sizeof(Pix)) % 4) % 4), f);
    }

    fclose(f);
    return arr;
}

void // +
print_info(BitmapFileHeader bmfh, BitmapInfoHeader bmif) {
    printf("headerSize:\t%x (%u)\n", bmif.headerSize, bmif.headerSize);
    printf("width: \t%x (%u)\n", bmif.width, bmif.width);
    printf("height: \t%x (%u)\n", bmif.height, bmif.height);
    printf("planes: \t%x (%hu)\n", bmif.planes, bmif.planes);
    printf("bitsPerPixel:\t%x (%hu)\n", bmif.bitsPerPixel, bmif.bitsPerPixel);
    printf("compression:\t%x (%u)\n", bmif.compression, bmif.compression);
    printf("imageSize:\t%x (%u)\n", bmif.imageSize, bmif.imageSize);
    printf("xPixelsPerMeter:\t%x (%u)\n", bmif.xPixelsPerMeter, bmif.xPixelsPerMeter);
    printf("yPixelsPerMeter:\t%x (%u)\n", bmif.yPixelsPerMeter, bmif.yPixelsPerMeter);
    printf("colorsInColorTable:\t%x (%u)\n", bmif.colorsInColorTable, bmif.colorsInColorTable);
    printf("importantColorCount:\t%x (%u)\n", bmif.importantColorCount, bmif.importantColorCount);

    printf("signature:\t%x (%hu)\n", bmfh.signature, bmfh.signature);
    printf("filesize:\t%x (%u)\n", bmfh.filesize, bmfh.filesize);
    printf("reserved1:\t%x (%hu)\n", bmfh.reserved1, bmfh.reserved1);
    printf("reserved2:\t%x (%hu)\n", bmfh.reserved2, bmfh.reserved2);
    printf("pixelArrOffset:\t%x (%u)\n", bmfh.pixelArrOffset, bmfh.pixelArrOffset);
}//+//+

void // +
write_bmp(char file_name[], Pix **arr, int H, int W, BitmapFileHeader bmfh, BitmapInfoHeader bmif) {
    FILE *ff = fopen(file_name, "wb");
    if (ff == NULL){
        printf("Error opening the output file.\n");
        exit(OPEN_OUTPUT_FILE);
    }

    fwrite(&bmfh, 1, sizeof(BitmapFileHeader), ff);
    fwrite(&bmif, 1, sizeof(BitmapInfoHeader), ff);
    for (int i = H - 1; i >= 0; i--) {
        fwrite(arr[i], 1, W * sizeof(Pix) + ((4 - (W * sizeof(Pix)) % 4) % 4), ff);
    }

    fclose(ff);
} //+//+

void // +
setPix(Pix** arr, int x, int y, long int* color){
    arr[y][x].r = color[2];
    arr[y][x].g = color[1];
    arr[y][x].b = color[0];
}

void
check_flag(int flag, int num){
    if (flag < num){
            printf("Invalid number of flags for function.\n");
            exit(INVALID_FLAGS);
        }
}

void // +
colorCircle(Pix** image, unsigned int H, unsigned int W, int ox, int oy, long int* color, long int rad){
    for (int x = -rad; x < rad ; x++) { // пр
        int height = round(sqrt(rad * rad - x * x));

        if (x + ox >= W){
            continue;
        }
        if (x + ox < 0){
            continue;
        }

        for (int y = -height; y < height; y++) {
            if (y + oy >= H){
                continue;
            }
            if (y + oy < 0){
                continue;
            }
            
            setPix(image, x + ox, y + oy, color);
        }
    }
}

void // +
void draw_line(Pix** image, unsigned int H, unsigned int W, long int start[2], long int end[2], long int color[3]) {
    int x = start[0];
    int y = start[1];

    const int deltaX = abs(end[0] - start[0]);
    const int deltaY = abs(end[1] - start[1]);
    const int signX = start[0] < end[0] ? 1 : -1;
    const int signY = start[1] < end[1] ? 1 : -1;

    int error = deltaX - deltaY;
    int error2;

    while (x != end[0] || y != end[1]) {
        if (x >= 0 && x < W && y >= 0 && y < H) {
            setPix(image, x, y, color);
        }

        error2 = error * 2;

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

void // +
write_line(Pix** image, unsigned int H, unsigned int W, long int x0, long int y0, long int xe, long int ye, long int color[3], long int thick){
    int rad = round(thick/2);
    colorCircle(image, H, W, x0, y0, color, rad);

    int dif[] = {xe - x0, ye - y0};
    float delta[2];
    int max_delta = 0;
    if (abs(dif[0]) >= abs(dif[1])){
        max_delta = abs(dif[0]);
    } else{
        max_delta = abs(dif[1]);
    }

    if (max_delta != 0){
        colorCircle(image, H, W, xe, ye, color, rad);

        delta[0] = dif[0] / max_delta;
        delta[1] = dif[1] / max_delta;

        delta[0] = delta[0] / (sqrt(delta[0]*delta[0] + delta[1]*delta[1]));
        delta[1] = delta[1] / (sqrt(delta[0]*delta[0] + delta[1]*delta[1]));

        rad = rad * 1.1;

        long int shtrih1_s[] = {round(x0 - rad * delta[1]), round(y0 + rad * delta[0]) - 1};
        long int shtrih1_e[] = {round(xe - rad * delta[1]), round(ye + rad * delta[0]) - 1};
        draw_line(image, H, W, shtrih1_s, shtrih1_e, color);

        long int shtrih2_s[] = {round(x0 + rad * delta[1]), round(y0 - rad * delta[0]) - 1};
        long int shtrih2_e[] = {round(xe + rad * delta[1]), round(ye - rad * delta[0]) - 1};

        const int deltaX = abs(shtrih2_s[0] - shtrih1_s[0]);
        const int deltaY = abs(shtrih2_s[1] - shtrih1_s[1]);
        const int signX = shtrih1_s[0] < shtrih2_s[0] ? 1 : -1;
        const int signY = shtrih1_s[1] < shtrih2_s[1] ? 1 : -1;
        int error = deltaX - deltaY;
        int x = shtrih1_s[0];
        int y = shtrih1_s[1];

        while ((x != shtrih2_s[0]) || (y != shtrih2_s[1])){
            long int start[] = {x, y};
            long int end[] = {x+dif[0], y+dif[1]};
            draw_line(image, H, W, start, end, color);
            if ((abs(delta[0]) <= 0.6) && ((start[0] < shtrih2_s[0] - 1) || (start[0] < shtrih1_s[0]))){
                start[0] += 1;
                end[0] += 1;
            } else if ((abs(delta[1]) < 0.6) && ((start[1] < shtrih2_s[1] - 1) || (start[1] < shtrih1_s[1]))){
                start[1] += 1;
                end[1] += 1;
            }
            draw_line(image, H, W, start, end, color);
            
            int error2 = error * 2;
            if(error2 > -deltaY) {
                error -= deltaY;
                x += signX;
            }
            if(error2 < deltaX) {
                error += deltaX;
                y += signY;
            }
        }
    }
}

void
invert_colors(Pix** image, unsigned int H, unsigned int W, long int center[2], long int rad){
    long int color[3];
    for (int i=-rad; i<=rad; i++){ //итерация по y
        for (int j=-rad; j<=rad; j++){ //итерация по x
            if ((i*i + j*j <= rad*rad) && (center[0]+j < W) && (center[1]+i < H) && (center[0]+j >= 0) && (center[1]+i >= 0)){
                color[2] = 255 - image[center[1]+i][center[0]+j].r;
                color[1] = 255 - image[center[1]+i][center[0]+j].g;
                color[0] = 255 - image[center[1]+i][center[0]+j].b;
                setPix(image, center[0]+j, center[1]+i, color);
            }
        }
    }
}

void
trim_image(Pix*** image, BitmapFileHeader** bmfh, BitmapInfoHeader** bmif, unsigned int H, unsigned int W, long int LU[2], long int RD[2]){
    unsigned int nH = LU[1] - RD[1];
    unsigned int nW = RD[0] - LU[0];

    if ((nH == 0) && (nW == 0)){
        nH = 1;
        nW = 1;
        Pix **arr = malloc(1 * sizeof(Pix*));

        arr[0] = malloc(nW * sizeof(Pix) + ((4 - (nW * sizeof(Pix)) % 4) % 4));
        arr[0][0] = (*image)[RD[1]][LU[0]];

        *image = arr;
    } else if((nH == 0) && (nW != 0)){
        nH = 1;

        Pix **arr = malloc(1 * sizeof(Pix*));
        arr[0] = malloc(nW * sizeof(Pix) + ((4 - (nW * sizeof(Pix)) % 4) % 4));

        for (int j = 0; j < nW; j++){
            arr[0][j] = (*image)[RD[1]][LU[0] + j];
        }

        *image = arr;
    } else if((nH != 0) && (nW == 0)){
        nW = 1;

        Pix **arr = malloc(nH * sizeof(Pix*));

        for (int i = 0; i < nH; i++){
            arr[i] = malloc(nW * sizeof(Pix) + ((4 - (nW * sizeof(Pix)) % 4) % 4));
            arr[i][0] = (*image)[RD[1] + i][LU[0]];
        }

        *image = arr;
    } else {
        Pix **arr = malloc(nH * sizeof(Pix*));

        for (int i = 0; i < nH; i++){
            arr[i] = malloc(nW * sizeof(Pix) + ((4 - (nW * sizeof(Pix)) % 4) % 4));
            for (int j = 0; j < nW; j++){
                arr[i][j] = (*image)[RD[1] + i][LU[0] + j];
            }
        }

        for (int i = 0; i < H; i++){
            free((*image)[i]);
        }

        *image = arr;
    }

    (*bmif)->height = nH;
    (*bmif)->width = nW;
}


int
main(int argc, char **argv){
    Line line;
    Inv_Cir inv_cir;
    Trim trim;
    BitmapInfoHeader* bmif = malloc(sizeof(BitmapInfoHeader));
    BitmapFileHeader* bmfh = malloc(sizeof(BitmapFileHeader));
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


    printf("Course work for option 4.6, created by Kavpush Vyacheslav\n");

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

    while ((c = getopt_long(argc, argv, "ls:e:c:t:vC:r:TL:R:i:o:Ih", long_options1, &optind)) != -1){
        switch(c){
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
                if (strstr(optarg, ".") == NULL){ //проверка наличия значения для аргумента
                    printf("Invalid input value.\n");
                    return 42;
                }
                line.start[0] = strtol(strtok(optarg, "."), NULL, 10);
                line.start[1] = strtol(strtok(NULL, "."), NULL, 10);
                break;
            case 'e':
                lin += 1;
                if (strstr(optarg, ".") == NULL){ //проверка наличия значения для аргумента
                    printf("Invalid input value.\n");
                    return 42;
                }
                line.end[0] = strtol(strtok(optarg, "."), NULL, 10);
                line.end[1] = strtol(strtok(NULL, "."), NULL, 10);
                break;
            case 'c':
                lin += 1;
                if (strstr(optarg, ".") == NULL){
                    printf("Invalid input value.\n");
                    return 42;
                }
                inpstr = strtok(optarg, ".");
                int count = 1;
                while (inpstr != NULL){
                    if (strtol(inpstr, NULL, 10) < 0 || strtol(inpstr, NULL, 10) > 255 || (strtol(inpstr, NULL, 10) == 0 && strcmp(inpstr, "0") != 0)){
                        printf("Invalid input value.\n");
                        return 42;
                    }
                    line.color[i] = strtol(inpstr, NULL, 10);
                    i+=1;
                    inpstr = strtok(NULL, ".");
                    count += 1;
                    if (inpstr == NULL && count != 4){
                        printf("Invalid input value.\n");
                        return 42;
                    }
                }
                break;
            case 't':
                lin += 1;
                if (strtol(optarg, NULL, 10) < 0 || strcmp(optarg, "0") == 0){
                    printf("Invalid input value.\n");
                    return 42;
                }
                line.thick = strtol(optarg, NULL, 10);
                break;
            case 'C':
                inv_c += 1;
                if (strstr(optarg, ".") == NULL){
                    printf("Invalid input value.\n");
                    return 42;
                }
                inv_cir.center[0] = strtol(strtok(optarg, "."), NULL, 10);
                inv_cir.center[1] = strtol(strtok(NULL, "."), NULL, 10);
                break;
            case 'r':
                inv_c += 1;
                inv_cir.rad = strtol(optarg, NULL, 10);
                break;
            case 'L':
                trm += 1;
                if (strstr(optarg, ".") == NULL){
                    printf("Invalid input value.\n");
                    return 42;
                }
                trim.leftUp[0] = strtol(strtok(optarg, "."), NULL, 10);
                trim.leftUp[1] = strtol(strtok(NULL, "."), NULL, 10);
                break;
            case 'R':
                trm += 1;
                if (strstr(optarg, ".") == NULL){
                    printf("Invalid input value.\n");
                    return 42;
                }
                trim.rightDown[0] = strtol(strtok(optarg, "."), NULL, 10);
                trim.rightDown[1] = strtol(strtok(NULL, "."), NULL, 10);
                break;
            case 'i':
                filename = realloc(filename, (strlen(optarg)+1)*sizeof(char));
                strcpy(filename, optarg);
                mark_f_inp = 1;
                break;
            case 'o':
                output_file = realloc(output_file, sizeof(char) * (strlen(optarg)+1));
                strcpy(output_file, optarg);
                mark_f_out = 1;
                break;
            case 'I':
                info_mark = 1;
                break;
            case 'h':
                printf("Flag (line) draws a line from one point to another, which initialize with flags (start) and (end) they need an argument (x.y). Color of the line initialize with flag (color) which requires argument (rrr.ggg.bbb), thickness requires flag (thickness).\n");
                printf("Flag (inverse_circle) invert colors in a circle with a center in a point initialized with flag (center), that requires argument in format (x.y), and radius initialized with flag (radius).\n");
                printf("Flag (trim) cuts the image so it will fit in rectangle with upper left corner in a point initialized with flag (left_up) and lower right corner in a point initialized with flag (right_down). They require argument in the format (x.y).\n");
                return 0;
            default:
                printf("Unknown option or missing argument.\n");
                return UNKNOWN_OPTION_OR_MIS_ARGUM;
        }
    }

    if ((mark_f_inp == 0) || (mark_f_out == 0)){
        if ((mark_f_inp == 0) && (mark_f_out != 0)){
            filename = realloc(filename, (strlen(argv[optind]) + 1) * sizeof(char));
            strcpy(filename, argv[optind]);
        } else if ((mark_f_inp != 0) && (mark_f_out == 0)){
            output_file = realloc(output_file, (7 + 1) * sizeof(char));
            strcpy(output_file, "out.bmp");
        } else {
            filename = realloc(filename, (strlen(argv[optind]) + 1) * sizeof(char));
            output_file = realloc(output_file, (7 + 1) * sizeof(char));
            strcpy(output_file, "out.bmp");
            strcpy(filename, argv[optind]);
        }
        if (strcmp(filename, output_file) == 0){
            printf("Invalid input. Same names of input and output files.\n");
            return SAME_INPUT_OUTPUT;
        }
    }

    Pix** image = read_bmp(filename, bmfh, bmif); //передали указатель на первый байт в памяти

    if (lin > 0){
        check_flag(lin, 5);
        write_line(image, bmif->height, bmif->width, line.start[0], line.start[1], line.end[0], line.end[1], line.color, line.thick);
    } else if (inv_c > 0){
        check_flag(inv_c, 3);
        invert_colors(image, bmif->height, bmif->width, inv_cir.center, inv_cir.rad);
    } else if (trm > 0){
        check_flag(trm, 3);

        if (trim.leftUp[0] < 0)
            trim.leftUp[0] = 0;
        if (trim.leftUp[1] < 0)
            trim.leftUp[1] = 0;
        if (trim.rightDown[1] < 0)
            trim.rightDown[1] = 0;
        if (trim.rightDown[0] < 0)
            trim.rightDown[0] = 0;
        if (trim.leftUp[1] >= bmif->height)
            trim.leftUp[1] = bmif->height - 1;
        if (trim.leftUp[0] >= bmif->width)
            trim.leftUp[0] = bmif->width  - 1;
        if (trim.rightDown[0] >= bmif->width)
            trim.rightDown[0] = bmif->width  - 1;
         if (trim.rightDown[1] >= bmif->height)
            trim.rightDown[1] = bmif->height  - 1;

        if (trim.leftUp[0] > trim.rightDown[0]){
            long int tmp = trim.rightDown[0];
            trim.rightDown[0] = trim.leftUp[0];
            trim.leftUp[0] = tmp;
        }
        if (trim.leftUp[1] < trim.rightDown[1]){
            long int tmp = trim.rightDown[1];
            trim.rightDown[1] = trim.leftUp[1];
            trim.leftUp[1] = tmp;
        }

        trim_image(&image, &bmfh, &bmif, bmif->height, bmif->width, trim.leftUp, trim.rightDown);
    } else if (info_mark > 0){
        print_info(*bmfh, *bmif);
    }

    write_bmp(output_file, image, bmif->height, bmif->width, *bmfh, *bmif);

    free(filename);
    free(output_file);
    for (unsigned int k = 0; k < bmif->height - 1; ++k){
        free(image[k]);
    }
    free(image);
    free(bmfh);
    free(bmif);

    return 0;
}