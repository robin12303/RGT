## 실행 환경

* OS: Windows (PowerShell 기준)
* 빌드 도구: CMake + (Visual Studio Build Tools 또는 Visual Studio)
* Python(Problem4): 가상환경(.venv) + `requirements.txt`

각 문제 실행 후, 제출용 “터미널 출력 값” 파일이 생성됩니다.

---

## 스마트 포인터를 활용한 리소스 관리

### 빌드 및 실행 (출력 저장 포함)

```powershell
# 1) 빌드
cmake -S . -B build
cmake --build build --config Debug

# 2) 실행 파일 경로 자동 선택 (VS Generator면 build\Debug, 아니면 build\)
$exe = ".\build\Debug\problem1.exe"
if (!(Test-Path $exe)) { $exe = ".\build\problem1.exe" }

# 3) 실행 출력 저장 (터미널 + 파일)
& $exe | Tee-Object -FilePath terminal_output_problem1.txt

# 4) 생성된 로그 내용도 같이 덧붙이기(제출용 증거)
"`n---- error.log ----"  | Tee-Object -Append terminal_output_problem1.txt
Get-Content .\error.log  | Tee-Object -Append terminal_output_problem1.txt
"`n---- debug.log ----"  | Tee-Object -Append terminal_output_problem1.txt
Get-Content .\debug.log  | Tee-Object -Append terminal_output_problem1.txt
"`n---- info.log ----"   | Tee-Object -Append terminal_output_problem1.txt
Get-Content .\info.log   | Tee-Object -Append terminal_output_problem1.txt
```

생성 파일:

* `terminal_output_problem1.txt`

---

## 템플릿과 STL을 활용한 컨테이너 설계

Problem2는 한글 출력이 깨질 수 있어 UTF-8 설정 + cmd 리다이렉트로 “바이트 그대로” 파일에 저장합니다.

### 빌드 및 실행 (출력 저장 포함)

```powershell
# UTF-8 콘솔 설정
chcp 65001 | Out-Null
$utf8 = [System.Text.UTF8Encoding]::new($false)
[Console]::InputEncoding  = $utf8
[Console]::OutputEncoding = $utf8

# 빌드 로그 저장(UTF-8)
cmake -S . -B build 2>&1 | Out-File terminal_output_problem2.txt -Encoding utf8
cmake --build build --config Debug 2>&1 | Out-File terminal_output_problem2.txt -Append -Encoding utf8

# 실행 파일 경로 선택
$exe = ".\build\Debug\problem2.exe"
if (!(Test-Path $exe)) { $exe = ".\build\problem2.exe" }

# 화면 출력(확인용)
& $exe

# 파일 저장은 cmd 리다이렉트로 append (한글 깨짐 방지)
cmd /c "`"$exe`" >> terminal_output_problem2.txt 2>&1"
```

생성 파일:

* `terminal_output_problem2.txt`

---

## 멀티스레딩과함수형 프로그래밍을 활용한 병렬 처리리 

Problem3도 Problem2와 동일하게 UTF-8 설정 + cmd 리다이렉트로 출력 저장합니다.

### 빌드 및 실행 (출력 저장 포함)

```powershell
# UTF-8 콘솔 강제
chcp 65001 | Out-Null
$utf8 = [System.Text.UTF8Encoding]::new($false)
[Console]::InputEncoding  = $utf8
[Console]::OutputEncoding = $utf8

$proj = "problem3"
$out  = "terminal_output_$proj.txt"
Remove-Item $out -ErrorAction SilentlyContinue

# 빌드 로그 저장(UTF-8)
cmake -S . -B build 2>&1 | Out-File $out -Encoding utf8
cmake --build build --config Debug 2>&1 | Out-File $out -Append -Encoding utf8

# 실행 파일 경로
$exe = ".\build\Debug\$proj.exe"
if (!(Test-Path $exe)) { $exe = ".\build\$proj.exe" }

# 화면 출력(확인용)
& $exe

# 파일 저장(cmd 리다이렉트)
cmd /c "chcp 65001>nul & ""$exe"" >> ""$out"" 2>&1"
```

생성 파일:

* `terminal_output_problem3.txt`

---

## Flask/FastAPI를 활용한 RESTful API 서버 구현

### 설치 및 실행

가상환경 활성화 후 의존성 설치:

```powershell
python -m pip install -r requirements.txt
```

서버 실행:

```powershell
python -m uvicorn main:app --reload
```

API 문서(Swagger):

* `http://127.0.0.1:8000/docs`

### 수동 테스트(스크립트)

서버 실행 상태에서, 같은 폴더에서:

```powershell
.\manual_test.ps1
```

생성 파일:

* `manual_test_output.jsonl` (수동 테스트 결과 로그)

### 자동 테스트(pytest)

```powershell
python -m pytest -q
```

(pytest 결과를 파일로 저장)

```powershell
python -m pytest -q | Out-File terminal_output_problem4_pytest.txt -Encoding utf8
```

---

## 제출용 출력 파일 요약

* Problem1: `terminal_output_problem1.txt`
* Problem2: `terminal_output_problem2.txt`
* Problem3: `terminal_output_problem3.txt`
* Problem4:

  * 수동 테스트 로그: `manual_test_output.jsonl`
  * (pytest 로그: `terminal_output_problem4_pytest.txt`

---

