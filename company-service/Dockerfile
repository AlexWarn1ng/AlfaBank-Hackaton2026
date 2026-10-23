# Build
FROM maven:3.9-eclipse-temurin-21 AS build

WORKDIR /build

COPY pom.xml ./
COPY src ./src

RUN mvn -B -DskipTests package

# Runtime
FROM eclipse-temurin:21-jre

WORKDIR /app

COPY --from=build /build/target/*.jar /app/app.jar

EXPOSE 8888

ENTRYPOINT ["java", "-jar", "/app/app.jar"]
