#pragma once

module MIS;

namespace Omni {

public interface MISHeuristic {
    public float Compute(float pdf_x, float pdf_y);
}

public struct BalanceHeuristic : MISHeuristic {
    public float Compute(float pdf_x, float pdf_y) {
        return pdf_x / (pdf_x + pdf_y);
    }
}

public struct PowerHeuristic : MISHeuristic {
    public float Compute(float pdf_x, float pdf_y) {
        float PDF_X_Squared = pdf_x * pdf_x;
        return (PDF_X_Squared) / (PDF_X_Squared + pdf_y * pdf_y);
    }
}

}