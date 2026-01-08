from datetime import datetime, timedelta, timezone
from typing import Optional, List

from fastapi import FastAPI, Depends, HTTPException, status
from fastapi.security import OAuth2PasswordBearer, OAuth2PasswordRequestForm
from jose import jwt, JWTError
from passlib.context import CryptContext
from pydantic import BaseModel, Field
from sqlalchemy import create_engine, Column, Integer, String, DateTime, ForeignKey
from sqlalchemy.orm import sessionmaker, declarative_base, Session, relationship

# =========================
# Config
# =========================
SECRET_KEY = "CHANGE_ME_TO_SOMETHING_RANDOM"  
ALGORITHM = "HS256"
ACCESS_TOKEN_EXPIRE_MINUTES = 60

DATABASE_URL = "sqlite:///./library.db"

engine = create_engine(
    DATABASE_URL, connect_args={"check_same_thread": False}
)
SessionLocal = sessionmaker(bind=engine, autocommit=False, autoflush=False)
Base = declarative_base()

pwd_context = CryptContext(schemes=["bcrypt"], deprecated="auto")
oauth2_scheme = OAuth2PasswordBearer(tokenUrl="/auth/login")

app = FastAPI(title="Library API (RGT Assignment)")


# =========================
# DB Models
# =========================
class User(Base):
    __tablename__ = "users"
    id = Column(Integer, primary_key=True)
    username = Column(String(50), unique=True, index=True, nullable=False)
    password_hash = Column(String(255), nullable=False)
    role = Column(String(20), nullable=False, default="user")  # "user" or "admin"

    loans = relationship("Loan", back_populates="user")


class Book(Base):
    __tablename__ = "books"
    id = Column(Integer, primary_key=True)
    title = Column(String(200), nullable=False)
    author = Column(String(200), nullable=False)
    total_copies = Column(Integer, nullable=False, default=1)
    available_copies = Column(Integer, nullable=False, default=1)

    loans = relationship("Loan", back_populates="book")


class Loan(Base):
    __tablename__ = "loans"
    id = Column(Integer, primary_key=True)
    user_id = Column(Integer, ForeignKey("users.id"), nullable=False)
    book_id = Column(Integer, ForeignKey("books.id"), nullable=False)
    borrowed_at = Column(DateTime(timezone=True), nullable=False)
    returned_at = Column(DateTime(timezone=True), nullable=True)

    user = relationship("User", back_populates="loans")
    book = relationship("Book", back_populates="loans")


Base.metadata.create_all(bind=engine)


# =========================
# Pydantic Schemas (validation)
# =========================
class UserCreate(BaseModel):
    username: str = Field(min_length=3, max_length=50)
    password: str = Field(min_length=6, max_length=100)


class Token(BaseModel):
    access_token: str
    token_type: str = "bearer"


class BookCreate(BaseModel):
    title: str = Field(min_length=1, max_length=200)
    author: str = Field(min_length=1, max_length=200)
    total_copies: int = Field(ge=1, le=100)


class BookOut(BaseModel):
    id: int
    title: str
    author: str
    total_copies: int
    available_copies: int

    class Config:
        from_attributes = True


class LoanOut(BaseModel):
    id: int
    book_id: int
    borrowed_at: datetime
    returned_at: Optional[datetime]

    class Config:
        from_attributes = True


# =========================
# Dependencies
# =========================
def get_db():
    db = SessionLocal()
    try:
        yield db
    finally:
        db.close()


def hash_password(pw: str) -> str:
    return pwd_context.hash(pw)


def verify_password(pw: str, hashed: str) -> bool:
    return pwd_context.verify(pw, hashed)


def create_access_token(*, sub: str, username: str, role: str, expires_minutes: int) -> str:
    now = datetime.now(timezone.utc)
    payload = {
        "sub": sub,
        "username": username,
        "role": role,
        "iat": int(now.timestamp()),
        "exp": int((now + timedelta(minutes=expires_minutes)).timestamp()),
    }
    return jwt.encode(payload, SECRET_KEY, algorithm=ALGORITHM)


def get_current_user(db: Session = Depends(get_db), token: str = Depends(oauth2_scheme)) -> User:
    try:
        payload = jwt.decode(token, SECRET_KEY, algorithms=[ALGORITHM])
        user_id = payload.get("sub")
        if user_id is None:
            raise HTTPException(status_code=401, detail="Invalid token")
    except JWTError:
        raise HTTPException(status_code=401, detail="Invalid token")

    user = db.query(User).filter(User.id == int(user_id)).first()
    if not user:
        raise HTTPException(status_code=401, detail="User not found")
    return user


def require_admin(user: User = Depends(get_current_user)) -> User:
    if user.role != "admin":
        raise HTTPException(status_code=403, detail="Admin only")
    return user


# =========================
# Auth endpoints
# =========================
@app.post("/auth/signup", status_code=201)
def signup(body: UserCreate, db: Session = Depends(get_db)):
    exists = db.query(User).filter(User.username == body.username).first()
    if exists:
        raise HTTPException(status_code=409, detail="Username already exists")

    user = User(
        username=body.username,
        password_hash=hash_password(body.password),
        role="user",
    )
    db.add(user)
    db.commit()
    db.refresh(user)
    return {"id": user.id, "username": user.username, "role": user.role}


@app.post("/auth/login", response_model=Token)
def login(form: OAuth2PasswordRequestForm = Depends(), db: Session = Depends(get_db)):
    user = db.query(User).filter(User.username == form.username).first()
    if not user or not verify_password(form.password, user.password_hash):
        raise HTTPException(status_code=401, detail="Invalid username or password")

    token = create_access_token(
        sub=str(user.id),
        username=user.username,
        role=user.role,
        expires_minutes=ACCESS_TOKEN_EXPIRE_MINUTES,
    )
    return Token(access_token=token)


# (과제용) 관리자 계정 생성 엔드포인트: 제출 후엔 지우는 게 정상
@app.post("/auth/bootstrap-admin", status_code=201)
def bootstrap_admin(body: UserCreate, db: Session = Depends(get_db)):
    # 이미 admin 있으면 막아도 되지만 과제에서는 단순하게
    exists = db.query(User).filter(User.username == body.username).first()
    if exists:
        raise HTTPException(status_code=409, detail="Username already exists")

    user = User(
        username=body.username,
        password_hash=hash_password(body.password),
        role="admin",
    )
    db.add(user)
    db.commit()
    db.refresh(user)
    return {"id": user.id, "username": user.username, "role": user.role}


# =========================
# Book endpoints (POST/GET/DELETE)
# =========================
@app.post("/books", response_model=BookOut, status_code=201)
def create_book(body: BookCreate, db: Session = Depends(get_db), _: User = Depends(require_admin)):
    book = Book(
        title=body.title,
        author=body.author,
        total_copies=body.total_copies,
        available_copies=body.total_copies,
    )
    db.add(book)
    db.commit()
    db.refresh(book)
    return book


@app.get("/books", response_model=List[BookOut])
def list_books(db: Session = Depends(get_db)):
    return db.query(Book).order_by(Book.id.asc()).all()


@app.get("/books/{book_id}", response_model=BookOut)
def get_book(book_id: int, db: Session = Depends(get_db)):
    book = db.query(Book).filter(Book.id == book_id).first()
    if not book:
        raise HTTPException(status_code=404, detail="Book not found")
    return book


@app.delete("/books/{book_id}", status_code=204)
def delete_book(book_id: int, db: Session = Depends(get_db), _: User = Depends(require_admin)):
    book = db.query(Book).filter(Book.id == book_id).first()
    if not book:
        raise HTTPException(status_code=404, detail="Book not found")
    db.delete(book)
    db.commit()
    return None


# =========================
# Loan endpoints (borrow/return)
# =========================
@app.post("/loans/borrow/{book_id}", status_code=201)
def borrow_book(book_id: int, db: Session = Depends(get_db), user: User = Depends(get_current_user)):
    book = db.query(Book).filter(Book.id == book_id).first()
    if not book:
        raise HTTPException(status_code=404, detail="Book not found")

    if book.available_copies <= 0:
        raise HTTPException(status_code=409, detail="No available copies")

    loan = Loan(
        user_id=user.id,
        book_id=book.id,
        borrowed_at=datetime.now(timezone.utc),
        returned_at=None,
    )
    book.available_copies -= 1

    db.add(loan)
    db.commit()
    db.refresh(loan)
    return {"loan_id": loan.id, "book_id": book.id, "borrowed_at": loan.borrowed_at}


@app.post("/loans/return/{book_id}", status_code=200)
def return_book(book_id: int, db: Session = Depends(get_db), user: User = Depends(get_current_user)):
    loan = (
        db.query(Loan)
        .filter(Loan.book_id == book_id, Loan.user_id == user.id, Loan.returned_at.is_(None))
        .order_by(Loan.borrowed_at.desc())
        .first()
    )
    if not loan:
        raise HTTPException(status_code=404, detail="Active loan not found")

    book = db.query(Book).filter(Book.id == book_id).first()
    if not book:
        raise HTTPException(status_code=404, detail="Book not found")

    loan.returned_at = datetime.now(timezone.utc)
    book.available_copies += 1

    db.commit()
    return {"loan_id": loan.id, "book_id": book.id, "returned_at": loan.returned_at}


@app.get("/me/loans", response_model=List[LoanOut])
def my_loans(db: Session = Depends(get_db), user: User = Depends(get_current_user)):
    return (
        db.query(Loan)
        .filter(Loan.user_id == user.id)
        .order_by(Loan.borrowed_at.desc())
        .all()
    )
