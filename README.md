# RPIproject

## 프로젝트 소개

- TCP 를 이용한 원격 장치 제어 프로그램 만들기

- 서버 : 라즈베리파이4
- 클라이언트 : 우분투 리눅스


## 구현내용

### 하드웨어 핀 연결

- LED : GPIO 17

- BUZZER : GPIO 25

- SEGMENT : 회로도 기준, D GPIO23 / C GPIO18 / B GPIO15 / A GPIO14

- CDS : SCL GPIO 3 | SDA GPIO 2

### 서버
- 멀티 스레드를 활용한 각 장치 제어 및 멀티유저 환경 구현

- LED on, off 및 밝기 조절(LOW, MID, HIGH)

- 클라이언트에서 음악소리 on, off 제어

- 클라이언트에서 조도 센서 값 확인
- 빛이 감지되지 않으면 LED on, 밫이 감지되면 LED off

- 클라이언트에서 전송한 숫자가 7-세그먼트 표시
- 1초 경과 시 1 씩 값이 감소하여 0이 되면 부저 울림.

- **웹서버 형식 구현**을 통해 웹 브라우저에서 접속 및 장치제어 가능.
- 뮤텍스를 통해 여러 명령 입력에 대해 순차적인 장치 제어가 가능하도록 설계

### 클라이언트
- 서버 접속 및 명령창에 들어온 알맞은 입력을 서버로 전송.
- 입력 받은 숫자 기반으로 웹 서버 프로토콜에 맞추어 메시지 전송

## 빌드 방식

1. server/makefile 내 HOSTNAME, TARGET_IP 입력

![alt text](resources/1.png)

HOSTNAME : 라즈베리파이 내 계정명 입력
TARGET_IP : 라즈베리파이 IP 주소 입력

2. 프로젝트 홈 디렉토리의 최상위 Makefile 실행

![alt text](resources/2.png)

- 최상위 메이크파일이 server/client의 메이크파일을 실행시킴
- server 및 client의 실행파일 및 라이브러리가 생성
- 서버파일들은 라즈베리파이의 홈 디렉토리로 전송됨(scp 이용)

3. 라즈베리파이 서버 동작시키기

- 라즈베리파이 서버 동작 전, 동적라이브러리들의 위치를 명시해줘야함.

`export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${HOME}"`

- 현재 실행파일과 동적 라이브러리 파일들이 위치한 라즈베리파이의 홈디렉토리를 LD_LIBRARY_PATH 환경변수에 등록.

![alt text](resources/3.png)

제대로 동작 시 데몬 프로세스로 서버가 동작하게 된다.

4. 우분투 리눅스에서 클라이언트 실행하기

이후 프로젝트 홈 디렉토리에 생성된 Client 실행파일을 실행

`Usage : ./Client <IP address> <port>

![alt text](resources/4.png)

서버와 잘 연결되었을 경우, 메뉴창이 나타나게 된다.

## CLI 환경 클라이언트 실행 과정

### 1. LED

![alt text](resources/5.png)

- 메뉴에서 1을 입력해주면 LED에 대한 상세옵션 메뉴가 뜨고, LED불빛 선택하여 켜기와 불끄기를 선택할 수 있음.

- 이상한 값 입력 시, Invalid input 이라는 문구가 뜨며 메인메뉴로 돌아간다.

### 2. Buzzer

![alt text](resources/6.png)

- 메인메뉴에서 2번 Buzzer 선택 후 1 - ON, 2 - OFF 로 음악을 켜고끌수 있음.

### 3. 조도센서

- 조도센서 선택 시 입력받은 그 순간의 환경의 광량을 감지하여 일정 수준 이하면 LED가 켜지고 이상이면 LED가 꺼짐.

- 광량 정보가 클라이언트로 리턴됨.

![alt text](resources/7.png)

### 4. 7-세그먼트

![alt text](resources/8.png)

- 메인메뉴 4번 선택 -> 세그먼트 시작 숫자값 기입 (0 ~ 9 범위)
- 1초마다 숫자값이 1 감소하여 0이 되면 부저가 울림.

### 5. exit

- 연결된 소켓을 닫고 정리하고 클라이언트를 종료함.

## 웹 브라우저 환경 클라이언트 동작 과정

- 라즈베리파이의 IP주소, 포트번호를 통해 웹 브라우저에서 서버 접속이 가능하다.

ex : `http://192.168.0.105:8081/`

![alt text](resources/9.png)

- 버튼을 누르면 장치 제어를 할 수 있음.


## 전체 프로그램 동작 흐름

```
[Client]                     [Server - Raspberry Pi]
   ↓                                  ↑
사용자 입력 (번호 선택)                |
   ↓                                  |
명령 문자열 생성 (GET /led_on 등)      |
   ↓                                  |
TCP 연결 및 전송 -------------------> 서버 수신 소켓 (멀티스레드)
                                      ↓
                            명령 파싱 → 적절한 장치 제어 스레드 생성
                                      ↓
                           LED / Buzzer / 조도 / 세그먼트 제어
                                      ↓
                        사용자에게 HTTP 응답 반환 <----------------
```

---

## 소스코드 파일별 설명

### Client 측

| 파일명             | 설명                                                            |
| --------------- | ------------------------------------------------------------- |
| `client.c`      | 클라이언트의 메인 루프. 사용자 메뉴 표시 → 입력 수신 → `write()`로 명령 전송 → 응답 수신.   |
| `buildMessge.c` | 사용자의 숫자 입력을 기반으로 HTTP 형식의 메시지를 조립 (`GET /led_on HTTP/1.1` 등). |

---

### Server 측

#### `server.c`

* 전체 서버의 메인 함수 담당
* 데몬 초기화 → TCP 소켓 바인딩 → 클라이언트 접속시 `pthread_create()`로 처리
* 요청별로 적절한 문자열을 파싱하여, LED/부저/조도/세그 스레드 중 하나를 실행

#### `buzz.c` / `buzz_thread.c`

* 부저 관련 기능
* `buzz.c`: 학교 종 멜로디 구현 (음계 배열)
* `buzz_thread.c`: 스레드로 부저 소리 재생 (`softToneWrite()` 사용)

#### `led.c` / `led_thread.c`

* LED 밝기 조절 기능
* `led.c`: `softPwmWrite(GPIO17, value)`로 LED 밝기 설정
* `led_thread.c`: `led_on`, `led_off`, `led_light` 명령 처리

#### `pr.c` / `pr_thread.c`

* 조도 센서 기능
* `pr.c`: GPIO23에서 센서 입력값 확인 (`digitalRead`)
* `pr_thread.c`: 조도 값에 따라 자동으로 LED ON/OFF 제어

#### `seg.c` / `seg_thread.c`

* 7-세그먼트 숫자 표시 기능
* `seg.c`: 각 숫자별 핀 조합 정의 (A\~G segment 핀 출력)
* `seg_thread.c`: 숫자 입력 → 1초 간격으로 감소 → 0이 되면 부저 스레드 실행

---

## 서버 스레드 동작 요약

| 장치     | 스레드 함수          | 내부 동작                                     |
| ------ | --------------- | ----------------------------------------- |
| LED    | `led_thread()`  | `led_on()`, `led_off()`, `led_light()` 호출 |
| Buzzer | `buzz_thread()` | 학교종 멜로디 음계 반복                             |
| 조도센서   | `pr_thread()`   | 조도 값 0/1 → 자동 LED ON/OFF                  |
| 세그먼트   | `seg_thread()`  | 숫자 출력 후 1초마다 감소, 종료 시 부저 울림               |

