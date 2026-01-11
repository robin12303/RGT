# UTF-8 출력 세팅(한글/인코딩 문제 예방)
chcp 65001 | Out-Null
$utf8 = [System.Text.UTF8Encoding]::new($false)
[Console]::InputEncoding  = $utf8
[Console]::OutputEncoding = $utf8

$base = "http://127.0.0.1:8000"
$log  = "manual_test_output.jsonl"   # json lines 느낌으로 쌓임
Remove-Item $log -ErrorAction SilentlyContinue

function Log-Line([string]$s) {
  $s | Out-File $log -Append -Encoding utf8
}

function Log-Obj([string]$label, $obj) {
  Log-Line ("{0}`t{1}" -f $label, ($obj | ConvertTo-Json -Depth 10 -Compress))
}

function Call-IRM([string]$label, [scriptblock]$call) {
  try {
    $res = & $call
    Log-Obj $label $res
    return $res
  } catch {
    # 에러도 로그로 남기기
    Log-Line ("ERROR`t{0}`t{1}" -f $label, $_.Exception.Message)
    throw
  }
}

# 1) 관리자 생성
Call-IRM "bootstrap_admin" {
  Invoke-RestMethod -Method Post "$base/auth/bootstrap-admin" `
    -ContentType "application/json" `
    -Body (@{ username="admin"; password="admin1234" } | ConvertTo-Json) `
    -ErrorAction Stop
} | Out-Host

# 2) 관리자 로그인(토큰은 로그에 전체 저장 안 하고 길이만 남김)
$adminLogin = Call-IRM "admin_login" {
  Invoke-RestMethod -Method Post "$base/auth/login" `
    -ContentType "application/x-www-form-urlencoded" `
    -Body "username=admin&password=admin1234" `
    -ErrorAction Stop
}
$adminToken = $adminLogin.access_token
Log-Line ("admin_token_len`t{0}" -f $adminToken.Length)
$adminToken | Out-Host

# 3) 책 생성
$book = Call-IRM "create_book" {
  Invoke-RestMethod -Method Post "$base/books" `
    -Headers @{ Authorization = "Bearer $adminToken" } `
    -ContentType "application/json" `
    -Body (@{ title="Clean Code"; author="Robert C. Martin"; total_copies=1 } | ConvertTo-Json) `
    -ErrorAction Stop
}
$bookId = $book.id
Log-Line ("book_id`t{0}" -f $bookId)
$book | Out-Host

# 4) 유저 생성
Call-IRM "signup_user" {
  Invoke-RestMethod -Method Post "$base/auth/signup" `
    -ContentType "application/json" `
    -Body (@{ username="user1"; password="pass1234" } | ConvertTo-Json) `
    -ErrorAction Stop
} | Out-Host

# 5) 유저 로그인(토큰도 길이만 로그)
$userLogin = Call-IRM "user_login" {
  Invoke-RestMethod -Method Post "$base/auth/login" `
    -ContentType "application/x-www-form-urlencoded" `
    -Body "username=user1&password=pass1234" `
    -ErrorAction Stop
}
$userToken = $userLogin.access_token
Log-Line ("user_token_len`t{0}" -f $userToken.Length)
$userToken | Out-Host

# 6) 대출/반납/조회
Call-IRM "borrow" { Invoke-RestMethod -Method Post "$base/loans/borrow/$bookId" -Headers @{ Authorization="Bearer $userToken" } -ErrorAction Stop } | Out-Host
Call-IRM "return" { Invoke-RestMethod -Method Post "$base/loans/return/$bookId" -Headers @{ Authorization="Bearer $userToken" } -ErrorAction Stop } | Out-Host
Call-IRM "my_loans" { Invoke-RestMethod -Method Get "$base/me/loans" -Headers @{ Authorization="Bearer $userToken" } -ErrorAction Stop } | Out-Host

"Saved: $log"
