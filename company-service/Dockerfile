# Build the Spring Boot service without requiring a source-tree reorganization.
FROM maven:3.9-eclipse-temurin-21 AS build
WORKDIR /build
COPY pom.xml ./
COPY *.java ./src/main/java/com/example/feature_extractor/
COPY application.properties ./src/main/resources/application.properties
RUN mvn -B -DskipTests package

FROM eclipse-temurin:21-jre
WORKDIR /app
COPY --from=build /build/target/*.jar /app/app.jar
EXPOSE 8888
ENTRYPOINT ["java", "-jar", "/app/app.jar"]
