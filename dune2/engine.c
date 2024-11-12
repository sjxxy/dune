#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include "common.h"
#include "io.h"
#include "display.h"
//TEST
void init(void);
void intro(void);
void outro(void);
void cursor_move(DIRECTION dir);
void sample_obj_move(void);
POSITION sample_obj_next_position(void);

/* ================= control =================== */
int sys_clock = 0;  // system-wide clock(ms)
CURSOR cursor = { { 1, 1 }, {1, 1} };

/* ================= game data =================== */
char map[N_LAYER][MAP_HEIGHT][MAP_WIDTH] = { 0 };

RESOURCE resource = {  // [MODIFIED]
    .spice = 10,        // [MODIFIED]
    .spice_max = 100,   // [MODIFIED]
    .population = 5,    // [MODIFIED]
    .population_max = 50 // [MODIFIED]
};

OBJECT_SAMPLE obj = {
    .pos = {1, 1},
    .dest = {MAP_HEIGHT - 2, MAP_WIDTH - 2},
    .repr = 'o',
    .speed = 300,
    .next_move_time = 300
};

/* ================= main() =================== */
int main(void) {
    srand((unsigned int)time(NULL));
//없앨 거
    init();
    intro();
    display(resource, map, cursor);  // [MODIFIED] Added arguments to match updated display() function signature

    while (1) {
        // loop 돌 때마다(즉, TICK==10ms마다) 키 입력 확인
        KEY key = get_key();

        // 키 입력이 있으면 처리
        if (is_arrow_key(key)) {
            cursor_move(ktod(key));
        }
        else {
            // 방향키 외의 입력
            switch (key) {
            case k_quit: outro();
            case k_none:
            case k_undef:
            default: break;
            }
        }

        // 샘플 오브젝트 동작
        sample_obj_move();

        // 화면 출력
        display(resource, map, cursor);  // [MODIFIED] Added arguments to match updated display() function signature
        Sleep(TICK);
        sys_clock += 10;
    }
}

/* ================= subfunctions =================== */
void intro(void) {
    printf("DUNE 1.5\n");
    Sleep(2000);
    system("cls");
}

void outro(void) {
    printf("exiting...\n");
    exit(0);
}

void init(void) {
    // 초기 상태 설정 [UPDATED WITH BOUNDARY IN MIND]
    // 좌하단 본진(B) 배치 (2x2 형태로 위치 조정)
    map[0][MAP_HEIGHT - 3][1] = 'B';
    map[0][MAP_HEIGHT - 3][2] = 'B';
    map[0][MAP_HEIGHT - 2][1] = 'B';
    map[0][MAP_HEIGHT - 2][2] = 'B';
    // 죄하단 장판(P) 배치
    map[0][MAP_HEIGHT - 3][3] = 'P';
    map[0][MAP_HEIGHT - 2][3] = 'P';
    map[0][MAP_HEIGHT - 3][4] = 'P';
    map[0][MAP_HEIGHT - 2][4] = 'P';
    // 우상단 본진(B) 배치
    map[0][1][MAP_WIDTH - 2] = 'B';
    map[0][2][MAP_WIDTH - 2] = 'B';
    map[0][1][MAP_WIDTH - 3] = 'B';
    map[0][2][MAP_WIDTH - 3] = 'B';
    // 좌하단 하베스터(H) 배치
    map[1][MAP_HEIGHT - 4][1] = 'H';

    // 우상단 하베스터(H) 배치
    map[1][3][MAP_WIDTH - 2] = 'H';

    // 좌하단 스파이스(5) 배치
    map[0][MAP_HEIGHT - 6][1] = 'S';

    // 우상단 스파이스(5) 배치
    map[0][3][MAP_WIDTH - 5] = 'S';

    // 바위(R) 배치 (맵 내 곳곳에 배치, 테두리 고려)
    map[0][5][11] = 'R';
    map[0][11][14] = 'R';
    map[0][9][19] = 'R';
    map[0][13][6] = 'R';

    // 샌드웜(W) 배치 (중앙 배치)
    map[1][MAP_HEIGHT / 2][MAP_WIDTH / 2] = 'W';

    // layer 0(map[0])에 지형 생성
    // 외곽 벽 생성
    for (int j = 0; j < MAP_WIDTH; j++) {
        map[0][0][j] = '#';
        map[0][MAP_HEIGHT - 1][j] = '#';
    }
    for (int i = 1; i < MAP_HEIGHT - 1; i++) {
        map[0][i][0] = '#';
        map[0][i][MAP_WIDTH - 1] = '#';
    }

    // 빈 공간 초기화
    for (int i = 1; i < MAP_HEIGHT - 1; i++) {
        for (int j = 1; j < MAP_WIDTH - 1; j++) {
            if (map[0][i][j] == 0) {
                map[0][i][j] = ' ';
            }
        }
    }

    // layer 1(map[1])은 비워 두기(-1로 채움)
    for (int i = 0; i < MAP_HEIGHT; i++) {
        for (int j = 0; j < MAP_WIDTH; j++) {
            if (map[1][i][j] == 0) {
                map[1][i][j] = -1;
            }
        }
    }

    // 초기 자원 설정 [ADDED]
    resource.spice = 10;
    resource.spice_max = 100;
    resource.population = 5;
    resource.population_max = 50;
}

// (가능하다면) 지정한 방향으로 커서 이동
void cursor_move(DIRECTION dir) {
    POSITION curr = cursor.current;
    POSITION new_pos = pmove(curr, dir);

    // validation check
    if (1 <= new_pos.row && new_pos.row <= MAP_HEIGHT - 2 && \
        1 <= new_pos.column && new_pos.column <= MAP_WIDTH - 2) {

        cursor.previous = cursor.current;
        cursor.current = new_pos;
    }
}

/* ================= sample object movement =================== */
POSITION sample_obj_next_position(void) {
    // 현재 위치와 목적지를 비교해서 이동 방향 결정    
    POSITION diff = psub(obj.dest, obj.pos);
    DIRECTION dir;

    // 목적지 도착. 지금은 단순히 원래 자리로 왕복
    if (diff.row == 0 && diff.column == 0) {
        if (obj.dest.row == 1 && obj.dest.column == 1) {
            // topleft --> bottomright로 목적지 설정
            POSITION new_dest = { MAP_HEIGHT - 2, MAP_WIDTH - 2 };
            obj.dest = new_dest;
        }
        else {
            // bottomright --> topleft로 목적지 설정
            POSITION new_dest = { 1, 1 };
            obj.dest = new_dest;
        }
        return obj.pos;
    }

    // 가로축, 세로축 거리를 비교해서 더 먼 쪽 축으로 이동
    if (abs(diff.row) >= abs(diff.column)) {
        dir = (diff.row >= 0) ? d_down : d_up;
    }
    else {
        dir = (diff.column >= 0) ? d_right : d_left;
    }

    // validation check
    // next_pos가 맵을 벗어나지 않고, (지금은 없지만)장애물에 부딪히지 않으면 다음 위치로 이동
    POSITION next_pos = pmove(obj.pos, dir);
    if (1 <= next_pos.row && next_pos.row <= MAP_HEIGHT - 2 && \
        1 <= next_pos.column && next_pos.column <= MAP_WIDTH - 2 && \
        map[1][next_pos.row][next_pos.column] < 0) {

        return next_pos;
    }
    else {
        return obj.pos;  // 제자리
    }
}

void sample_obj_move(void) {
    if (sys_clock <= obj.next_move_time) {
        // 아직 시간이 안 됐음
        return;
    }

    // 오브젝트(건물, 유닛 등)은 layer1(map[1])에 저장
    map[1][obj.pos.row][obj.pos.column] = -1;
    obj.pos = sample_obj_next_position();
    map[1][obj.pos.row][obj.pos.column] = obj.repr;

    obj.next_move_time = sys_clock + obj.speed;
}
