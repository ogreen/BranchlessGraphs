fit.over.all.graphs <- function (Vars, Df.fit, Df.predict
                               , const.term=FALSE, nonneg=TRUE)
{
  response.var <- if (Vars$has.cycles) "Cycles" else "Time"
  response.true <- sprintf ("%s.true", response.var)
  Data.predicted <- NULL
  Predictions <- NULL

  Fits <- list ()
  Models <- list ()
  Predictors.all <- list ()
  for (arch in unique (Df.fit[["Architecture"]])) {
    for (alg in unique (Df.fit[["Algorithm"]])) {
      for (code in unique (Df.fit[["Implementation"]])) {
        cat (sprintf ("==> %s for %s on %s ...\n", code, alg, arch))
    
        # Choose subset of data to fit
        Data.fit <- subset (Df.fit
                            , Architecture == arch &
                              Algorithm == alg &
                              Implementation == code)

        # Determine predictors
        vars.key <- get.file.suffix (arch, alg, code)
        vars.file <- sprintf ("figs2/explore-corr-vars--%s.txt", vars.key)
        cat (sprintf ("    Loading predictors from: %s ...\n", vars.file))
        if (!file.exists (vars.file)) {
          stop (sprintf ("\n*** Missing model file: '%s' ***\n", vars.file))
        }
        Predictors <- as.vector (unlist (read.table (vars.file)))

        # Fit!
        Fit <- lm.by.colnames (Data.fit, response.var, Predictors
                               , const.term=const.term, nonneg=nonneg)

#        cat (sprintf ("\n=== Fitted model for: %s code for %s on %s ===\n", code, alg, arch))
#        print (summary (Fit))

        # Use fitted model to predict totals
        Data.predict <- subset (Df.predict, Architecture == arch &
                                            Algorithm == alg &
                                            Implementation == code)
        Prediction <- predict.df.lm (Fit, Data.predict, response.var)
        Prediction[, response.true] <- Data.predict[, response.var]
        Prediction <- cbind (Data.predict[, Vars$Index], Prediction)
      
#        cat (sprintf ("\n=== Sample predictions ===\n"))
#        print (head (Prediction))

        Predictions <- rbind.fill (Predictions, Prediction)
        Data.predicted <- rbind.fill (Data.predicted, Data.predict)

        mod.key <- vars.key
        Predictors.all[[mod.key]] <- Predictors
        Models[[mod.key]] <- Fit
      } # for each code
    } # for each alg
  } # for each arch

  Fits$Data.predicted <- Data.predicted
  Fits$Predictions <- Predictions
  Fits$response.var <- response.var
  Fits$response.true <- response.true
  Fits$Predictors.all <- Predictors.all
  Fits$Models <- Models
  return (Fits)
}

fit.one.per.graph <- function (Vars, Df.fit, Df.predict
                               , const.term=FALSE, nonneg=TRUE)
{
  response.var <- if (Vars$has.cycles) "Cycles" else "Time"
  response.true <- sprintf ("%s.true", response.var)
  Data.predicted <- NULL
  Predictions <- NULL

  Fits <- list ()
  Models <- list ()
  Predictors.all <- list ()
  for (arch in unique (Df.fit[["Architecture"]])) {
    for (alg in unique (Df.fit[["Algorithm"]])) {
      for (code in unique (Df.fit[["Implementation"]])) {
        # Determine predictors
        vars.key <- get.file.suffix (arch, alg, code)
        vars.file <- sprintf ("figs2/explore-corr-vars--%s.txt", vars.key)
        cat (sprintf ("    Loading predictors from: %s ...\n", vars.file))
        if (!file.exists (vars.file)) {
          stop (sprintf ("\n*** Missing model file: '%s' ***\n", vars.file))
        }
        Predictors <- as.vector (unlist (read.table (vars.file)))

        # Fit a model per graph
        for (graph in unique (Df.fit[["Graph"]])) {
#          cat (sprintf ("==> %s for %s on %s with input %s ...\n", code, alg, arch, graph))
    
          # Choose subset of training data to fit
          Data.fit <- subset (Df.fit
                              , Architecture == arch &
                                Algorithm == alg &
                                Implementation == code &
                                Graph == graph)

          # Choose subset of testing data to predict
          Data.predict <- subset (Df.predict, Architecture == arch &
                                              Algorithm == alg &
                                              Implementation == code &
                                              Graph == graph)
          
          # Don't even try if there aren't enough samples
          if (nrow (Data.fit) < 2) {
            warning (sprintf ("Insufficient samples (n=%d) to fit: %s code for %s on %s with input %s\n"
                              , nrow (Data.fit)
                              , code, alg, arch, graph))
            Fit <- list ()
            Prediction <- Data.predict[response.var]
            Prediction[, response.true] <- Data.predict[, response.var]
            Prediction[, Predictors] <- 0
            Prediction <- cbind (Data.predict[, Vars$Index], Prediction)
          } else {
            # Fit!
            Fit <- lm.by.colnames (Data.fit, response.var, Predictors
                                   , const.term=const.term, nonneg=nonneg)

#            cat (sprintf ("\n=== Fitted model for: %s code for %s on %s with input %s ===\n", code, alg, arch, graph))
#            print (summary (Fit))

            Prediction <- predict.df.lm (Fit, Data.predict, response.var)
            Prediction[, response.true] <- Data.predict[, response.var]
            Prediction <- cbind (Data.predict[, Vars$Index], Prediction)
          }
      
#          cat (sprintf ("\n=== Sample predictions ===\n"))
#          print (head (Prediction))

          Predictions <- rbind.fill (Predictions, Prediction)

          mod.key <- sprintf ("%s--%s", vars.key, graph)
          Predictors.all[[mod.key]] <- Predictors
          Models[[mod.key]] <- Fit
        } # for each graph
      } # for each code
    } # for each alg
  } # for each arch

  Fits$Data.predicted <- Data.predicted
  Fits$Predictions <- Predictions
  Fits$response.var <- response.var
  Fits$response.true <- response.true
  Fits$Predictors.all <- Predictors.all
  Fits$Models <- Models
  return (Fits)
}

# eof

