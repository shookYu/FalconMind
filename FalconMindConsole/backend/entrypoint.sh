#!/bin/bash
set -e

echo "🚀 FalconMind Console Backend Starting..."

# Wait for database to be ready
echo "⏳ Waiting for database..."
until PGPASSWORD=$DB_PASSWORD psql -h "$DB_HOST" -U "$DB_USER" -d "$DB_NAME" -c '\q'; do
  echo "   Database is unavailable - sleeping"
  sleep 1
done
echo "✅ Database is up"

# Run Alembic migrations
echo "🔄 Running database migrations..."
alembic upgrade head

# Check if we need to initialize data
if [ "$AUTO_INIT_DATA" = "true" ]; then
    echo "🌱 Initializing default data..."
    python scripts/init_data.py
fi

# Execute the main command
echo "🎯 Starting application..."
exec "$@"
