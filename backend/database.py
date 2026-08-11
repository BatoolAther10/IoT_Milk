from sqlalchemy import create_engine
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import sessionmaker

# Database file location
SQLALCHEMY_DATABASE_URL = "sqlite:///./lab_data.db"

# Engine configuration with thread check disabled and a 20-second timeout for concurrent writes
engine = create_engine(
    SQLALCHEMY_DATABASE_URL,
    connect_args={
        "check_same_thread": False, # Allows multiple threads to interact with SQLite
        "timeout": 20                # Wait up to 20s if DB is busy writing another request
    }
)

# Session factory for DB dependency injection
SessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=engine)

# Base class for SQLAlchemy models
Base = declarative_base()

# Dependency to provide a database session per HTTP request
def get_db():
    db = SessionLocal()
    try:
        yield db
    finally:
        db.close()