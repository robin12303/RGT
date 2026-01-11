import pytest
from fastapi.testclient import TestClient
from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker
from sqlalchemy.pool import StaticPool

from main import app, Base, get_db


# -------------------------
# Test DB (in-memory SQLite)
# -------------------------
TEST_DATABASE_URL = "sqlite+pysqlite://"

engine = create_engine(
    TEST_DATABASE_URL,
    connect_args={"check_same_thread": False},
    poolclass=StaticPool,
)
TestingSessionLocal = sessionmaker(bind=engine, autocommit=False, autoflush=False)


def override_get_db():
    db = TestingSessionLocal()
    try:
        yield db
    finally:
        db.close()


app.dependency_overrides[get_db] = override_get_db


@pytest.fixture(autouse=True)
def setup_db():
    Base.metadata.create_all(bind=engine)
    yield
    Base.metadata.drop_all(bind=engine)


@pytest.fixture()
def client():
    return TestClient(app)


# -------------------------
# Helpers
# -------------------------
def bootstrap_admin(client, username="admin", password="admin1234"):
    r = client.post("/auth/bootstrap-admin", json={"username": username, "password": password})
    assert r.status_code == 201, r.text
    return r.json()


def signup(client, username="user1", password="pass1234"):
    r = client.post("/auth/signup", json={"username": username, "password": password})
    assert r.status_code == 201, r.text
    return r.json()


def login(client, username, password):
    # OAuth2PasswordRequestForm => form data
    r = client.post(
        "/auth/login",
        data={"username": username, "password": password},
        headers={"Content-Type": "application/x-www-form-urlencoded"},
    )
    assert r.status_code == 200, r.text
    token = r.json()["access_token"]
    return {"Authorization": f"Bearer {token}"}


def create_book_as_admin(client, admin_headers, title="Clean Code", author="Robert C. Martin", total_copies=1):
    r = client.post(
        "/books",
        json={"title": title, "author": author, "total_copies": total_copies},
        headers=admin_headers,
    )
    assert r.status_code == 201, r.text
    return r.json()


# -------------------------
# Tests
# -------------------------
def test_admin_can_create_book(client):
    bootstrap_admin(client, "admin", "admin1234")
    admin_headers = login(client, "admin", "admin1234")

    book = create_book_as_admin(client, admin_headers, total_copies=2)
    assert book["total_copies"] == 2
    assert book["available_copies"] == 2


def test_user_cannot_create_book(client):
    signup(client, "user1", "pass1234")
    user_headers = login(client, "user1", "pass1234")

    r = client.post(
        "/books",
        json={"title": "Should Fail", "author": "Nope", "total_copies": 1},
        headers=user_headers,
    )
    assert r.status_code == 403, r.text


def test_borrow_return_flow(client):
    bootstrap_admin(client, "admin", "admin1234")
    admin_headers = login(client, "admin", "admin1234")
    book = create_book_as_admin(client, admin_headers, total_copies=1)
    book_id = book["id"]

    signup(client, "user1", "pass1234")
    user_headers = login(client, "user1", "pass1234")

    # borrow success
    r = client.post(f"/loans/borrow/{book_id}", headers=user_headers)
    assert r.status_code == 201, r.text

    # borrow again -> no copies
    r = client.post(f"/loans/borrow/{book_id}", headers=user_headers)
    assert r.status_code == 409, r.text

    # return
    r = client.post(f"/loans/return/{book_id}", headers=user_headers)
    assert r.status_code == 200, r.text
    assert r.json()["returned_at"] is not None

    # list my loans
    r = client.get("/me/loans", headers=user_headers)
    assert r.status_code == 200, r.text
    loans = r.json()
    assert len(loans) == 1
    assert loans[0]["book_id"] == book_id


def test_invalid_login_returns_401(client):
    r = client.post(
        "/auth/login",
        data={"username": "ghost", "password": "whatever"},
        headers={"Content-Type": "application/x-www-form-urlencoded"},
    )
    assert r.status_code == 401, r.text


def test_signup_rejects_short_username(client):
    # username min_length=3 이므로 422가 정상
    r = client.post("/auth/signup", json={"username": "u1", "password": "pass1234"})
    assert r.status_code == 422, r.text
