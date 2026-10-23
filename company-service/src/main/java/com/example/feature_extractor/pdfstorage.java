package com.example.feature_extractor;
import jakarta.persistence.*;
import java.time.OffsetDateTime;

@Entity
@Table(name = "pdf_documents")
public class pdfstorage {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    @Column(name = "filename", nullable = false, columnDefinition = "TEXT")
    private String filename;

    @Column(name = "content", nullable = false, columnDefinition = "bytea")
    private byte[] content;

    @Column(name = "uploaded_at")
    private OffsetDateTime uploadedAt;

    public pdfstorage() {}

    public Long getId() {
        return id;
    }

    public String getFilename() {
        return filename;
    }

    public void setFilename(String filename) {
        this.filename = filename;
    }

    public byte[] getContent() {
        return content;
    }

    public void setContent(byte[] content) {
        this.content = content;
    }

    public OffsetDateTime getUploadedAt() {
        return uploadedAt;
    }
}